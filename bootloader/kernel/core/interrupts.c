// AION OS Advanced Interrupt Handling System
#include "interrupts.h"
#include "../drivers/pic.h"
#include "../drivers/apic.h"
#include "../ai/predictor.h"

// Interrupt descriptor table
static idt_entry_t idt[256] __attribute__((aligned(16)));
static idt_ptr_t idt_ptr;

// Interrupt statistics for AI optimization
static interrupt_stats_t interrupt_stats[256] = {0};
static ai_interrupt_predictor_t *int_predictor;

// Interrupt handler registry
static interrupt_handler_t interrupt_handlers[256] = {0};

// Initialize interrupt system
void interrupts_init(void) {
    kprintf("[INTERRUPTS] Initializing interrupt system...\n");
    
    // Disable interrupts during setup
    asm volatile("cli");
    
    // Initialize IDT entries
    memset(idt, 0, sizeof(idt));
    
    // Set up exception handlers (0-31)
    for (int i = 0; i < 32; i++) {
        set_idt_gate(i, (uint64_t)exception_handlers[i], 
                     KERNEL_CS, IDT_INTERRUPT_GATE, 0);
    }
    
    // Set up IRQ handlers (32-47)
    for (int i = 32; i < 48; i++) {
        set_idt_gate(i, (uint64_t)irq_handlers[i - 32],
                     KERNEL_CS, IDT_INTERRUPT_GATE, 0);
    }
    
    // Set up system calls (128)
    set_idt_gate(128, (uint64_t)syscall_handler,
                 KERNEL_CS, IDT_INTERRUPT_GATE, 3);  // Ring 3 accessible
    
    // Set up IPI handlers for SMP (240-255)
    for (int i = 240; i < 256; i++) {
        set_idt_gate(i, (uint64_t)ipi_handlers[i - 240],
                     KERNEL_CS, IDT_INTERRUPT_GATE, 0);
    }
    
    // Load IDT
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;
    load_idt(&idt_ptr);
    
    // Initialize PIC/APIC
    if (detect_apic()) {
        apic_init();
        kprintf("[INTERRUPTS] Using APIC\n");
    } else {
        pic_init();
        kprintf("[INTERRUPTS] Using legacy PIC\n");
    }
    
    // Initialize AI interrupt predictor
    int_predictor = ai_interrupt_predictor_create();
    
    // Enable interrupts
    asm volatile("sti");
    
    kprintf("[INTERRUPTS] Interrupt system initialized\n");
}

// Set IDT gate
void set_idt_gate(uint8_t num, uint64_t handler, uint16_t selector,
                  uint8_t flags, uint8_t dpl) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0;
    idt[num].flags = flags | (dpl << 5) | IDT_PRESENT;
    idt[num].reserved = 0;
}

// Register interrupt handler
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler) {
    if (num < 256) {
        interrupt_handlers[num] = handler;
        kprintf("[INTERRUPTS] Registered handler for interrupt %d\n", num);
    }
}

// Common interrupt dispatcher
void interrupt_dispatcher(interrupt_frame_t *frame) {
    uint8_t int_num = frame->int_num;
    
    // Update statistics
    interrupt_stats[int_num].count++;
    interrupt_stats[int_num].last_time = get_system_time();
    
    // AI prediction: Should we batch this interrupt?
    if (int_predictor->should_batch(int_num)) {
        // Queue interrupt for batch processing
        queue_interrupt(frame);
        return;
    }
    
    // Call registered handler
    if (interrupt_handlers[int_num]) {
        interrupt_handlers[int_num](frame);
    } else {
        kprintf("[INTERRUPTS] Unhandled interrupt %d\n", int_num);
    }
    
    // Send EOI
    send_eoi(int_num);
    
    // AI learning: Record interrupt handling time
    uint64_t handling_time = get_system_time() - interrupt_stats[int_num].last_time;
    int_predictor->record_handling_time(int_num, handling_time);
}

// Exception handlers
__attribute__((interrupt))
void divide_by_zero_handler(interrupt_frame_t *frame) {
    kprintf("[EXCEPTION] Divide by zero at RIP: 0x%llx\n", frame->rip);
    dump_interrupt_frame(frame);
    kernel_panic("Divide by zero exception");
}

__attribute__((interrupt))
void page_fault_handler(interrupt_frame_t *frame, uint64_t error_code) {
    uint64_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
    
    kprintf("[EXCEPTION] Page fault at 0x%llx\n", faulting_address);
    kprintf("  Error code: 0x%llx\n", error_code);
    kprintf("  Present: %d, Write: %d, User: %d\n",
            error_code & 0x1, (error_code >> 1) & 0x1, (error_code >> 2) & 0x1);
    
    // Try to handle page fault
    if (handle_page_fault(faulting_address, error_code)) {
        return;  // Successfully handled
    }
    
    dump_interrupt_frame(frame);
    kernel_panic("Page fault");
}

__attribute__((interrupt))
void general_protection_fault_handler(interrupt_frame_t *frame, uint64_t error_code) {
    kprintf("[EXCEPTION] General Protection Fault\n");
    kprintf("  Error code: 0x%llx\n", error_code);
    dump_interrupt_frame(frame);
    kernel_panic("General Protection Fault");
}

// IRQ handlers
__attribute__((interrupt))
void timer_irq_handler(interrupt_frame_t *frame) {
    // Update system time
    system_tick();
    
    // AI-optimized scheduling decision
    if (should_reschedule()) {
        schedule();
    }
    
    send_eoi(IRQ_TIMER);
}

__attribute__((interrupt))
void keyboard_irq_handler(interrupt_frame_t *frame) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Process keyboard input
    keyboard_process_scancode(scancode);
    
    send_eoi(IRQ_KEYBOARD);
}

// System call handler
__attribute__((interrupt))
void syscall_handler(interrupt_frame_t *frame) {
    // System call number in RAX
    // Arguments in RDI, RSI, RDX, R10, R8, R9
    
    uint64_t syscall_num = frame->rax;
    uint64_t result = 0;
    
    // Dispatch system call
    switch (syscall_num) {
        case SYS_READ:
            result = sys_read(frame->rdi, (void*)frame->rsi, frame->rdx);
            break;
        case SYS_WRITE:
            result = sys_write(frame->rdi, (void*)frame->rsi, frame->rdx);
            break;
        case SYS_OPEN:
            result = sys_open((char*)frame->rdi, frame->rsi);
            break;
        case SYS_CLOSE:
            result = sys_close(frame->rdi);
            break;
        case SYS_FORK:
            result = sys_fork();
            break;
        case SYS_EXEC:
            result = sys_exec((char*)frame->rdi, (char**)frame->rsi);
            break;
        case SYS_EXIT:
            sys_exit(frame->rdi);
            break;
        case SYS_GETPID:
            result = sys_getpid();
            break;
        case SYS_MMAP:
            result = sys_mmap((void*)frame->rdi, frame->rsi, frame->rdx);
            break;
        default:
            kprintf("[SYSCALL] Unknown syscall: %lld\n", syscall_num);
            result = -1;
    }
    
    // Return result in RAX
    frame->rax = result;
}

// Dump interrupt frame for debugging
void dump_interrupt_frame(interrupt_frame_t *frame) {
    kprintf("\n[INTERRUPT FRAME]\n");
    kprintf("  RAX: 0x%016llx  RBX: 0x%016llx\n", frame->rax, frame->rbx);
    kprintf("  RCX: 0x%016llx  RDX: 0x%016llx\n", frame->rcx, frame->rdx);
    kprintf("  RSI: 0x%016llx  RDI: 0x%016llx\n", frame->rsi, frame->rdi);
    kprintf("  RBP: 0x%016llx  RSP: 0x%016llx\n", frame->rbp, frame->rsp);
    kprintf("  R8:  0x%016llx  R9:  0x%016llx\n", frame->r8, frame->r9);
    kprintf("  R10: 0x%016llx  R11: 0x%016llx\n", frame->r10, frame->r11);
    kprintf("  R12: 0x%016llx  R13: 0x%016llx\n", frame->r12, frame->r13);
    kprintf("  R14: 0x%016llx  R15: 0x%016llx\n", frame->r14, frame->r15);
    kprintf("  RIP: 0x%016llx  CS:  0x%04x\n", frame->rip, frame->cs);
    kprintf("  RFLAGS: 0x%016llx\n", frame->rflags);
    kprintf("  INT: %d  ERR: 0x%llx\n", frame->int_num, frame->error_code);
}