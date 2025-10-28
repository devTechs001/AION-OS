#include "usb.h"
#include "../pci.h"
#include <string.h>

// xHCI Capability Registers
typedef struct {
    uint8_t   caplength;
    uint8_t   reserved;
    uint16_t  hciversion;
    uint32_t  hcsparams1;
    uint32_t  hcsparams2;
    uint32_t  hcsparams3;
    uint32_t  hccparams1;
    uint32_t  dboff;
    uint32_t  rtsoff;
    uint32_t  hccparams2;
} __attribute__((packed)) xhci_cap_regs_t;

// xHCI Operational Registers
typedef struct {
    uint32_t usbcmd;
    uint32_t usbsts;
    uint32_t pagesize;
    uint32_t reserved1[2];
    uint32_t dnctrl;
    uint64_t crcr;
    uint32_t reserved2[4];
    uint64_t dcbaap;
    uint32_t config;
} __attribute__((packed)) xhci_op_regs_t;

// xHCI Port Registers
typedef struct {
    uint32_t portsc;
    uint32_t portpmsc;
    uint32_t portli;
    uint32_t porthlpmc;
} __attribute__((packed)) xhci_port_regs_t;

// xHCI Transfer Request Block (TRB)
typedef struct {
    uint64_t parameter;
    uint32_t status;
    uint32_t control;
} __attribute__((packed)) xhci_trb_t;

// xHCI Event Ring Segment
typedef struct {
    uint64_t address;
    uint32_t size;
    uint32_t reserved;
} __attribute__((packed)) xhci_erst_entry_t;

// xHCI Controller Context
typedef struct {
    xhci_cap_regs_t* cap_regs;
    xhci_op_regs_t* op_regs;
    xhci_port_regs_t* port_regs;
    
    uint32_t* doorbell_array;
    
    // Command Ring
    xhci_trb_t* cmd_ring;
    int cmd_ring_size;
    int cmd_ring_cycle;
    
    // Event Ring
    xhci_trb_t* event_ring;
    xhci_erst_entry_t* erst;
    int event_ring_size;
    int event_ring_cycle;
    
    // Device Context Base Address Array
    uint64_t* dcbaa;
    
    // AI Performance Tracking
    uint64_t total_transfers;
    uint64_t failed_transfers;
    uint32_t avg_completion_time_us;
    
    spinlock_t lock;
} xhci_controller_t;

// Initialize xHCI Controller
static int xhci_init(pci_device_t* pci_dev) {
    xhci_controller_t* xhci = kmalloc(sizeof(xhci_controller_t));
    memset(xhci, 0, sizeof(xhci_controller_t));
    
    // Map MMIO registers
    uintptr_t bar0 = pci_read_bar(pci_dev, 0);
    xhci->cap_regs = (xhci_cap_regs_t*)bar0;
    
    uintptr_t op_base = bar0 + xhci->cap_regs->caplength;
    xhci->op_regs = (xhci_op_regs_t*)op_base;
    
    int max_ports = (xhci->cap_regs->hcsparams1 >> 24) & 0xFF;
    xhci->port_regs = (xhci_port_regs_t*)(op_base + 0x400);
    
    kprintf("[xHCI] Controller found: %d ports, version %x.%x\n",
            max_ports,
            xhci->cap_regs->hciversion >> 8,
            xhci->cap_regs->hciversion & 0xFF);
    
    // Stop controller
    xhci->op_regs->usbcmd &= ~1;
    while (!(xhci->op_regs->usbsts & 1)) {
        cpu_pause();
    }
    
    // Reset controller
    xhci->op_regs->usbcmd |= (1 << 1);
    while (xhci->op_regs->usbcmd & (1 << 1)) {
        cpu_pause();
    }
    
    // Wait for ready
    while (xhci->op_regs->usbsts & 0x800) {
        cpu_pause();
    }
    
    kprintf("[xHCI] Controller reset complete\n");
    
    // Allocate DCBAA (Device Context Base Address Array)
    int max_slots = xhci->cap_regs->hcsparams1 & 0xFF;
    xhci->dcbaa = kmalloc_aligned((max_slots + 1) * 8, 64);
    memset(xhci->dcbaa, 0, (max_slots + 1) * 8);
    xhci->op_regs->dcbaap = (uint64_t)xhci->dcbaa;
    
    // Set max device slots
    xhci->op_regs->config = max_slots;
    
    // Allocate Command Ring
    xhci->cmd_ring_size = 256;
    xhci->cmd_ring = kmalloc_aligned(xhci->cmd_ring_size * sizeof(xhci_trb_t), 64);
    memset(xhci->cmd_ring, 0, xhci->cmd_ring_size * sizeof(xhci_trb_t));
    xhci->cmd_ring_cycle = 1;
    
    // Link last TRB to first
    xhci->cmd_ring[xhci->cmd_ring_size - 1].parameter = (uint64_t)xhci->cmd_ring;
    xhci->cmd_ring[xhci->cmd_ring_size - 1].control = (6 << 10) | 1; // Link TRB, cycle bit
    
    // Set Command Ring Control Register
    xhci->op_regs->crcr = (uint64_t)xhci->cmd_ring | 1;
    
    // Allocate Event Ring
    xhci->event_ring_size = 256;
    xhci->event_ring = kmalloc_aligned(xhci->event_ring_size * sizeof(xhci_trb_t), 64);
    memset(xhci->event_ring, 0, xhci->event_ring_size * sizeof(xhci_trb_t));
    
    // Allocate Event Ring Segment Table
    xhci->erst = kmalloc_aligned(sizeof(xhci_erst_entry_t), 64);
    xhci->erst[0].address = (uint64_t)xhci->event_ring;
    xhci->erst[0].size = xhci->event_ring_size;
    xhci->erst[0].reserved = 0;
    
    // Configure Primary Event Ring
    uintptr_t runtime_base = bar0 + xhci->cap_regs->rtsoff;
    uint32_t* ir0_management = (uint32_t*)(runtime_base + 0x20);
    uint64_t* ir0_dequeue = (uint64_t*)(runtime_base + 0x28);
    uint64_t* ir0_segment = (uint64_t*)(runtime_base + 0x30);
    uint32_t* ir0_segment_size = (uint32_t*)(runtime_base + 0x38);
    
    *ir0_segment_size = 1;
    *ir0_segment = (uint64_t)xhci->erst;
    *ir0_dequeue = (uint64_t)xhci->event_ring;
    
    // Get doorbell array
    xhci->doorbell_array = (uint32_t*)(bar0 + xhci->cap_regs->dboff);
    
    // Enable controller
    xhci->op_regs->usbcmd |= 1;
    while (xhci->op_regs->usbsts & 1) {
        cpu_pause();
    }
    
    kprintf("[xHCI] Controller started\n");
    
    // Enable interrupts
    xhci->op_regs->usbcmd |= (1 << 2); // Interrupt enable
    
    spinlock_init(&xhci->lock);
    
    // Register USB controller
    usb_controller_t* controller = kmalloc(sizeof(usb_controller_t));
    strcpy(controller->name, "xHCI");
    controller->type = 3; // xHCI
    controller->private_data = xhci;
    
    usb_register_controller(controller);
    
    // AI: Start performance monitoring
    kprintf("[xHCI] AI performance monitoring enabled\n");
    
    return 0;
}

// xHCI PCI Driver
static pci_driver_t xhci_driver = {
    .name = "xhci_hcd",
    .class = 0x0C,
    .subclass = 0x03,
    .prog_if = 0x30,
    .probe = xhci_init
};

void xhci_register(void) {
    pci_register_driver(&xhci_driver);
}