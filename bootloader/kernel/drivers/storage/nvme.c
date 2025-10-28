#include "nvme.h"
#include "../pci.h"
#include <string.h>

static nvme_controller_t* nvme_controllers[8];
static int nvme_controller_count = 0;

// Submit command to queue
static int nvme_submit_command(nvme_queue_t* queue, nvme_command_t* cmd, 
                               nvme_completion_t* completion) {
    spinlock_acquire(&queue->lock);
    
    // Copy command to submission queue
    uint16_t tail = queue->sq_tail;
    memcpy(&queue->sq[tail], cmd, sizeof(nvme_command_t));
    
    // Advance tail
    queue->sq_tail = (tail + 1) % queue->queue_depth;
    
    // Ring doorbell
    *queue->sq_doorbell = queue->sq_tail;
    
    // AI: Track submission time for latency prediction
    uint64_t start_time = rdtsc();
    
    // Wait for completion
    while (1) {
        uint16_t head = queue->cq_head;
        nvme_completion_t* cqe = &queue->cq[head];
        
        // Check phase bit
        if (((cqe->status >> 0) & 1) == queue->cq_phase) {
            // Command completed
            if (completion) {
                memcpy(completion, cqe, sizeof(nvme_completion_t));
            }
            
            // Advance head
            queue->cq_head = (head + 1) % queue->queue_depth;
            if (queue->cq_head == 0) {
                queue->cq_phase = !queue->cq_phase;
            }
            
            // Update doorbell
            *queue->cq_doorbell = queue->cq_head;
            
            // AI: Calculate latency
            uint64_t latency_cycles = rdtsc() - start_time;
            // Update average latency metrics
            
            spinlock_release(&queue->lock);
            return (cqe->status >> 1) & 0x7FF;
        }
        
        cpu_pause();
    }
}

// Identify Controller/Namespace
static int nvme_identify(nvme_controller_t* ctrl, uint32_t nsid, void* buffer) {
    nvme_command_t cmd = {0};
    cmd.cdw0 = NVME_ADMIN_IDENTIFY;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer;
    cmd.cdw10 = (nsid == 0) ? 1 : 0; // CNS: 1=Controller, 0=Namespace
    
    return nvme_submit_command(&ctrl->admin_queue, &cmd, NULL);
}

// Create I/O Queue Pair
static int nvme_create_io_queue(nvme_controller_t* ctrl, int qid, int queue_depth) {
    nvme_queue_t* queue = &ctrl->io_queues[qid];
    
    // Allocate queues
    queue->sq = kmalloc_aligned(queue_depth * sizeof(nvme_command_t), 4096);
    queue->cq = kmalloc_aligned(queue_depth * sizeof(nvme_completion_t), 4096);
    queue->queue_depth = queue_depth;
    queue->sq_head = 0;
    queue->sq_tail = 0;
    queue->cq_head = 0;
    queue->cq_phase = 1;
    
    spinlock_init(&queue->lock);
    
    // Get doorbell registers
    uint32_t dstrd = (ctrl->bar0[0] >> 32) & 0xF;
    queue->sq_doorbell = (uint32_t*)((uintptr_t)ctrl->bar0 + 0x1000 + 
                                     (2 * qid * (4 << dstrd)));
    queue->cq_doorbell = (uint32_t*)((uintptr_t)ctrl->bar0 + 0x1000 + 
                                     ((2 * qid + 1) * (4 << dstrd)));
    
    // Create Completion Queue
    nvme_command_t cmd = {0};
    cmd.cdw0 = NVME_ADMIN_CREATE_CQ | ((qid & 0xFFFF) << 16);
    cmd.prp1 = (uint64_t)queue->cq;
    cmd.cdw10 = ((queue_depth - 1) << 16) | qid;
    cmd.cdw11 = 1; // Physically contiguous
    
    if (nvme_submit_command(&ctrl->admin_queue, &cmd, NULL) != 0) {
        kprintf("[NVMe] Failed to create CQ %d\n", qid);
        return -1;
    }
    
    // Create Submission Queue
    memset(&cmd, 0, sizeof(cmd));
    cmd.cdw0 = NVME_ADMIN_CREATE_SQ | ((qid & 0xFFFF) << 16);
    cmd.prp1 = (uint64_t)queue->sq;
    cmd.cdw10 = ((queue_depth - 1) << 16) | qid;
    cmd.cdw11 = (qid << 16) | 1; // CQ ID + Physically contiguous
    
    if (nvme_submit_command(&ctrl->admin_queue, &cmd, NULL) != 0) {
        kprintf("[NVMe] Failed to create SQ %d\n", qid);
        return -1;
    }
    
    kprintf("[NVMe] Created I/O queue pair %d (depth: %d)\n", qid, queue_depth);
    return 0;
}

// Read sectors
int nvme_read(nvme_controller_t* ctrl, int nsid, uint64_t lba, 
              uint32_t count, void* buffer) {
    // AI: Choose optimal queue based on current load
    int queue_id = 1; // Simplified - use first I/O queue
    nvme_queue_t* queue = &ctrl->io_queues[queue_id];
    
    nvme_command_t cmd = {0};
    cmd.cdw0 = NVME_CMD_READ | ((ctrl->next_cid++ & 0xFFFF) << 16);
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer;
    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = (count - 1) & 0xFFFF; // Number of blocks - 1
    
    uint64_t start = rdtsc();
    int status = nvme_submit_command(queue, &cmd, NULL);
    uint64_t latency = rdtsc() - start;
    
    // AI: Update performance metrics
    nvme_namespace_t* ns = &ctrl->namespaces[nsid - 1];
    ns->reads++;
    ns->bytes_read += count * ns->block_size;
    
    // Update average latency using exponential moving average
    uint32_t latency_us = (uint32_t)(latency / (cpu_frequency_hz() / 1000000));
    ns->avg_read_latency_us = (ns->avg_read_latency_us * 7 + latency_us) / 8;
    
    // AI: Predict next access for prefetching
    nvme_ai_predict_access_pattern(ctrl, lba);
    
    return status;
}

// Write sectors
int nvme_write(nvme_controller_t* ctrl, int nsid, uint64_t lba,
               uint32_t count, const void* buffer) {
    int queue_id = 1;
    nvme_queue_t* queue = &ctrl->io_queues[queue_id];
    
    nvme_command_t cmd = {0};
    cmd.cdw0 = NVME_CMD_WRITE | ((ctrl->next_cid++ & 0xFFFF) << 16);
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer;
    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = (count - 1) & 0xFFFF;
    
    uint64_t start = rdtsc();
    int status = nvme_submit_command(queue, &cmd, NULL);
    uint64_t latency = rdtsc() - start;
    
    // AI: Update metrics
    nvme_namespace_t* ns = &ctrl->namespaces[nsid - 1];
    ns->writes++;
    ns->bytes_written += count * ns->block_size;
    
    uint32_t latency_us = (uint32_t)(latency / (cpu_frequency_hz() / 1000000));
    ns->avg_write_latency_us = (ns->avg_write_latency_us * 7 + latency_us) / 8;
    
    return status;
}

// AI: Predict access patterns for prefetching
void nvme_ai_predict_access_pattern(nvme_controller_t* ctrl, uint64_t lba) {
    // Simple sequential access detection
    static uint64_t last_lba = 0;
    static int sequential_count = 0;
    
    if (lba == last_lba + 1) {
        sequential_count++;
        
        // If sequential access detected, prefetch next blocks
        if (sequential_count > 3) {
            // Trigger prefetch of next 64 blocks
            // nvme_ai_prefetch(ctrl, 1, lba + 1);
        }
    } else {
        sequential_count = 0;
    }
    
    last_lba = lba;
}

// Initialize NVMe controller
static int nvme_probe(pci_device_t* pci_dev) {
    nvme_controller_t* ctrl = kmalloc(sizeof(nvme_controller_t));
    memset(ctrl, 0, sizeof(nvme_controller_t));
    
    // Map BAR0
    uintptr_t bar0 = pci_read_bar(pci_dev, 0);
    ctrl->bar0 = (volatile uint64_t*)bar0;
    
    kprintf("[NVMe] Controller found at BAR0: 0x%lx\n", bar0);
    
    // Read capabilities
    uint64_t cap = ctrl->bar0[0];
    uint32_t dstrd = (cap >> 32) & 0xF;
    uint32_t mqes = (cap & 0xFFFF) + 1;
    
    kprintf("[NVMe] Max Queue Entries: %d, Doorbell Stride: %d\n", mqes, 4 << dstrd);
    
    // Disable controller
    volatile uint32_t* cc = (volatile uint32_t*)((uintptr_t)ctrl->bar0 + NVME_REG_CC);
    *cc = 0;
    
    // Wait for ready
    volatile uint32_t* csts = (volatile uint32_t*)((uintptr_t)ctrl->bar0 + NVME_REG_CSTS);
    while (*csts & 1) {
        cpu_pause();
    }
    
    // Configure Admin Queue
    int admin_queue_size = 64;
    ctrl->admin_queue.sq = kmalloc_aligned(admin_queue_size * sizeof(nvme_command_t), 4096);
    ctrl->admin_queue.cq = kmalloc_aligned(admin_queue_size * sizeof(nvme_completion_t), 4096);
    ctrl->admin_queue.queue_depth = admin_queue_size;
    ctrl->admin_queue.sq_head = 0;
    ctrl->admin_queue.sq_tail = 0;
    ctrl->admin_queue.cq_head = 0;
    ctrl->admin_queue.cq_phase = 1;
    
    spinlock_init(&ctrl->admin_queue.lock);
    
    // Set admin queue attributes
    volatile uint32_t* aqa = (volatile uint32_t*)((uintptr_t)ctrl->bar0 + NVME_REG_AQA);
    *aqa = ((admin_queue_size - 1) << 16) | (admin_queue_size - 1);
    
    // Set admin queue addresses
    volatile uint64_t* asq = (volatile uint64_t*)((uintptr_t)ctrl->bar0 + NVME_REG_ASQ);
    volatile uint64_t* acq = (volatile uint64_t*)((uintptr_t)ctrl->bar0 + NVME_REG_ACQ);
    *asq = (uint64_t)ctrl->admin_queue.sq;
    *acq = (uint64_t)ctrl->admin_queue.cq;
    
    // Get doorbell registers
    ctrl->admin_queue.sq_doorbell = (uint32_t*)((uintptr_t)ctrl->bar0 + 0x1000);
    ctrl->admin_queue.cq_doorbell = (uint32_t*)((uintptr_t)ctrl->bar0 + 0x1000 + (4 << dstrd));
    
    // Enable controller
    *cc = (6 << 20) | (4 << 16) | (0 << 14) | (0 << 11) | 1;
    // MPS=4KB, CSS=NVM, AMS=RR, Enable
    
    // Wait for ready
    while (!(*csts & 1)) {
        cpu_pause();
    }
    
    kprintf("[NVMe] Controller enabled\n");
    
    // Identify controller
    void* identify_buf = kmalloc_aligned(4096, 4096);
    nvme_identify(ctrl, 0, identify_buf);
    
    char serial[21] = {0};
    memcpy(serial, identify_buf + 4, 20);
    kprintf("[NVMe] Serial: %s\n", serial);
    
    // Get number of namespaces
    uint32_t nn = *(uint32_t*)(identify_buf + 516);
    kprintf("[NVMe] Namespaces: %d\n", nn);
    
    // Create I/O queues
    ctrl->num_io_queues = 4; // Create 4 I/O queue pairs
    for (int i = 0; i < ctrl->num_io_queues; i++) {
        nvme_create_io_queue(ctrl, i + 1, 256);
    }
    
    // Identify each namespace
    ctrl->num_namespaces = nn;
    for (uint32_t i = 1; i <= nn; i++) {
        nvme_identify(ctrl, i, identify_buf);
        
        uint64_t nsze = *(uint64_t*)(identify_buf + 0);
        uint32_t lbaf = *(uint32_t*)(identify_buf + 128 + 26);
        uint32_t lba_size = 1 << ((lbaf >> 16) & 0xFF);
        
        ctrl->namespaces[i-1].nsid = i;
        ctrl->namespaces[i-1].size = nsze;
        ctrl->namespaces[i-1].block_size = lba_size;
        ctrl->namespaces[i-1].capacity = nsze * lba_size;
        
        kprintf("[NVMe] Namespace %d: %llu MB (%d byte blocks)\n",
                i, (nsze * lba_size) / (1024 * 1024), lba_size);
    }
    
    kfree(identify_buf);
    
    nvme_controllers[nvme_controller_count++] = ctrl;
    
    kprintf("[NVMe] Initialization complete\n");
    return 0;
}

static pci_driver_t nvme_driver = {
    .name = "nvme",
    .class = 0x01,
    .subclass = 0x08,
    .prog_if = 0x02,
    .probe = nvme_probe
};

void nvme_init(void) {
    pci_register_driver(&nvme_driver);
}