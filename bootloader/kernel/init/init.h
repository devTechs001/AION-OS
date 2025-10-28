#ifndef INIT_H
#define INIT_H

#include <stdint.h>
#include <stdbool.h>

// Service States
#define SERVICE_STATE_STOPPED   0
#define SERVICE_STATE_STARTING  1
#define SERVICE_STATE_RUNNING   2
#define SERVICE_STATE_STOPPING  3
#define SERVICE_STATE_FAILED    4

// Service Types
#define SERVICE_TYPE_SIMPLE     0
#define SERVICE_TYPE_FORKING    1
#define SERVICE_TYPE_ONESHOT    2
#define SERVICE_TYPE_NOTIFY     3

// Service Structure
typedef struct service {
    char name[64];
    char description[256];
    char exec_path[256];
    char* args[32];
    
    int type;
    int state;
    int pid;
    
    // Dependencies
    struct service* requires[16];
    int num_requires;
    
    struct service* wants[16];
    int num_wants;
    
    struct service* after[16];
    int num_after;
    
    // Resource Limits
    uint64_t memory_limit;
    uint32_t cpu_limit_percent;
    
    // Statistics
    uint64_t start_time;
    uint64_t restarts;
    
    // AI Performance Monitoring
    struct {
        uint32_t avg_cpu_usage;
        uint64_t avg_memory_usage;
        uint32_t predicted_memory_needed;
        bool memory_leak_detected;
    } ai_stats;
    
    spinlock_t lock;
} service_t;

// Init System
typedef struct {
    service_t services[256];
    int num_services;
    
    int runlevel;
    
    spinlock_t lock;
} init_system_t;

// Function Prototypes
void init_system_start(void);
int init_load_service(const char* service_file);
int init_start_service(const char* service_name);
int init_stop_service(const char* service_name);
int init_restart_service(const char* service_name);
int init_enable_service(const char* service_name);
int init_disable_service(const char* service_name);

// Service Control
service_t* init_find_service(const char* name);
void init_update_service_state(service_t* service, int new_state);

// AI-Enhanced Features
void init_ai_monitor_services(void);
void init_ai_auto_restart(service_t* service);
void init_ai_optimize_startup_order(void);

#endif // INIT_H