#ifndef NVME_H
#define NVME_H

#include <stdint.h>
#include <stdbool.h>

// NVMe Register Offsets
#define NVME_REG_CAP        0x00
#define NVME_REG_VS         0x08
#define NVME_REG_CC         0x14
#define NVME_REG_CSTS       0x1C
#define NVME_REG_AQA        0x24
#define NVME_REG_ASQ        0x28
#define NVME_REG_ACQ        0x30

// NVMe Admin Commands
#define NVME_ADMIN_DELETE_SQ    0x00
#define NVME_ADMIN_CREATE_SQ    0x01
#define NVME_ADMIN_DELETE_CQ    0x04
#define NVME_ADMIN_CREATE_CQ    0x05
#define NVME_ADMIN_IDENTIFY     0x06
#define NVME_ADMIN_SET_FEATURES 0x09
#define NVME_ADMIN_GET_FEATURES 0x0A

// NVMe I/O Commands
#define NVME_CMD_FLUSH      0x00
#define NVME_CMD_WRITE      0x01
#define NVME_CMD_READ       0x02

// NVMe Submission Queue Entry
typedef struct {
    uint32_t cdw0;      // Command Dword 0
    uint32_t nsid;      // Namespace ID
    uint64_t reserved;
    uint64_t metadata;
    uint64_t prp1;      // Physical Region Page 1
    uint64_t prp2;      // Physical Region Page 2
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} __attribute__((packed)) nvme_command_t;

// NVMe Completion Queue Entry
typedef struct {
    uint32_t dw0;
    uint32_t dw1;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t cid;       // Command ID
    uint16_t status;
} __attribute__((packed)) nvme_completion_t;

// NVMe Queue Pair
typedef struct {
    nvme_command_t* sq;     // Submission Queue
    nvme_completion_t* cq;  // Completion Queue
    
    uint32_t* sq_doorbell;
    uint32_t* cq_doorbell;
    
    uint16_t sq_head;
    uint16_t sq_tail;
    uint16_t cq_head;
    uint16_t cq_phase;
    
    uint16_t queue_depth;
    
    spinlock_t lock;
} nvme_queue_t;

// NVMe Namespace
typedef struct {
    uint32_t nsid;
    uint64_t size;          // Size in blocks
    uint32_t block_size;
    uint64_t capacity;      // Total capacity in bytes
    
    // AI Performance Metrics
    uint64_t reads;
    uint64_t writes;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint32_t avg_read_latency_us;
    uint32_t avg_write_latency_us;
    
    // AI Predictions
    float predicted_iops;
    uint32_t predicted_latency_us;
    bool wear_leveling_needed;
} nvme_namespace_t;

// NVMe Controller
typedef struct {
    volatile uint64_t* bar0;    // Memory-mapped registers
    
    nvme_queue_t admin_queue;
    nvme_queue_t io_queues[64];
    int num_io_queues;
    
    nvme_namespace_t namespaces[256];
    int num_namespaces;
    
    uint16_t next_cid;
    
    // AI-Enhanced I/O Scheduler
    struct {
        uint64_t pending_reads;
        uint64_t pending_writes;
        uint32_t queue_depth;
        float predicted_throughput_mbps;
    } ai_scheduler;
    
    spinlock_t lock;
} nvme_controller_t;

// Function Prototypes
void nvme_init(void);
int nvme_read(nvme_controller_t* ctrl, int nsid, uint64_t lba, 
              uint32_t count, void* buffer);
int nvme_write(nvme_controller_t* ctrl, int nsid, uint64_t lba,
               uint32_t count, const void* buffer);
int nvme_flush(nvme_controller_t* ctrl, int nsid);

// AI-Enhanced Features
void nvme_ai_optimize_queue_depth(nvme_controller_t* ctrl);
void nvme_ai_predict_access_pattern(nvme_controller_t* ctrl, uint64_t lba);
void nvme_ai_prefetch(nvme_controller_t* ctrl, int nsid, uint64_t lba);

#endif // NVME_H