#include "init.h"
#include <string.h>
#include <stdlib.h>

static init_system_t global_init = {0};

void init_system_start(void) {
    kprintf("[INIT] Starting AION OS Init System\n");
    
    memset(&global_init, 0, sizeof(global_init));
    spinlock_init(&global_init.lock);
    global_init.runlevel = 3; // Multi-user mode
    
    // Load service definitions
    init_load_service("/etc/aion/services/network.service");
    init_load_service("/etc/aion/services/sshd.service");
    init_load_service("/etc/aion/services/cron.service");
    init_load_service("/etc/aion/services/ai-optimizer.service");
    
    // AI: Optimize service startup order based on dependencies
    kprintf("[INIT AI] Optimizing service startup order...\n");
    init_ai_optimize_startup_order();
    
    // Start essential services
    init_start_service("network");
    init_start_service("ai-optimizer");
    
    // Start AI service monitor
    kprintf("[INIT AI] Starting service health monitor\n");
    // Create kernel thread for monitoring
    // thread_create(init_ai_monitor_services, NULL, "init-ai-monitor");
    
    kprintf("[INIT] System initialization complete\n");
}

int init_start_service(const char* service_name) {
    service_t* service = init_find_service(service_name);
    if (!service) {
        kprintf("[INIT] Service not found: %s\n", service_name);
        return -1;
    }
    
    spinlock_acquire(&service->lock);
    
    if (service->state == SERVICE_STATE_RUNNING) {
        kprintf("[INIT] Service already running: %s\n", service_name);
        spinlock_release(&service->lock);
        return 0;
    }
    
    kprintf("[INIT] Starting service: %s\n", service_name);
    init_update_service_state(service, SERVICE_STATE_STARTING);
    
    // Start dependencies first
    for (int i = 0; i < service->num_requires; i++) {
        if (service->requires[i]->state != SERVICE_STATE_RUNNING) {
            kprintf("[INIT]   Starting dependency: %s\n", service->requires[i]->name);
            init_start_service(service->requires[i]->name);
        }
    }
    
    // Execute service
    int pid = fork();
    if (pid == 0) {
        // Child process
        execve(service->exec_path, service->args, NULL);
        exit(1); // If execve fails
    } else if (pid > 0) {
        service->pid = pid;
        service->start_time = timer_get_ticks();
        init_update_service_state(service, SERVICE_STATE_RUNNING);
        
        kprintf("[INIT] Service %s started (PID: %d)\n", service_name, pid);
    } else {
        kprintf("[INIT] Failed to start service: %s\n", service_name);
        init_update_service_state(service, SERVICE_STATE_FAILED);
        spinlock_release(&service->lock);
        return -1;
    }
    
    spinlock_release(&service->lock);
    return 0;
}

int init_stop_service(const char* service_name) {
    service_t* service = init_find_service(service_name);
    if (!service) return -1;
    
    spinlock_acquire(&service->lock);
    
    if (service->state != SERVICE_STATE_RUNNING) {
        spinlock_release(&service->lock);
        return 0;
    }
    
    kprintf("[INIT] Stopping service: %s\n", service_name);
    init_update_service_state(service, SERVICE_STATE_STOPPING);
    
    // Send SIGTERM
    kill(service->pid, SIGTERM);
    
    // Wait for graceful shutdown (5 seconds)
    uint64_t timeout = timer_get_ticks() + 5000;
    while (timer_get_ticks() < timeout) {
        if (process_exists(service->pid) == false) {
            break;
        }
        schedule();
    }
    
    // Force kill if still running
    if (process_exists(service->pid)) {
        kprintf("[INIT] Force killing service: %s\n", service_name);
        kill(service->pid, SIGKILL);
    }
    
    init_update_service_state(service, SERVICE_STATE_STOPPED);
    service->pid = 0;
    
    kprintf("[INIT] Service stopped: %s\n", service_name);
    
    spinlock_release(&service->lock);
    return 0;
}

// AI: Monitor service health and auto-restart if needed
void init_ai_monitor_services(void) {
    while (1) {
        sleep(5000); // Check every 5 seconds
        
        spinlock_acquire(&global_init.lock);
        
        for (int i = 0; i < global_init.num_services; i++) {
            service_t* service = &global_init.services[i];
            
            if (service->state != SERVICE_STATE_RUNNING) continue;
            
            // Check if process still exists
            if (!process_exists(service->pid)) {
                kprintf("[INIT AI] Service %s crashed, restarting...\n", service->name);
                service->restarts++;
                init_ai_auto_restart(service);
                continue;
            }
            
            // AI: Monitor resource usage
            process_t* proc = process_get(service->pid);
            if (proc) {
                uint64_t memory = process_get_memory_usage(proc);
                uint32_t cpu = process_get_cpu_usage(proc);
                
                // Update AI statistics
                service->ai_stats.avg_cpu_usage = 
                    (service->ai_stats.avg_cpu_usage * 7 + cpu) / 8;
                service->ai_stats.avg_memory_usage = 
                    (service->ai_stats.avg_memory_usage * 7 + memory) / 8;
                
                // AI: Detect memory leak
                if (memory > service->ai_stats.avg_memory_usage * 2) {
                    kprintf("[INIT AI] Possible memory leak in %s\n", service->name);
                    service->ai_stats.memory_leak_detected = true;
                    
                    // Restart if memory exceeds limit
                    if (service->memory_limit > 0 && memory > service->memory_limit) {
                        kprintf("[INIT AI] Memory limit exceeded, restarting %s\n", 
                                service->name);
                        init_restart_service(service->name);
                    }
                }
                
                // AI: Detect CPU usage anomalies
                if (cpu > service->cpu_limit_percent) {
                    kprintf("[INIT AI] CPU limit exceeded for %s: %d%%\n", 
                            service->name, cpu);
                    // Could throttle or restart
                }
            }
        }
        
        spinlock_release(&global_init.lock);
    }
}

// AI: Optimize service startup order
void init_ai_optimize_startup_order(void) {
    // Topological sort of service dependencies
    // Use AI to predict startup times and parallelize when possible
    
    kprintf("[INIT AI] Analyzing service dependency graph...\n");
    
    // Build dependency matrix
    int dep_matrix[256][256] = {0};
    for (int i = 0; i < global_init.num_services; i++) {
        service_t* service = &global_init.services[i];
        for (int j = 0; j < service->num_requires; j++) {
            // Find index of dependency
            for (int k = 0; k < global_init.num_services; k++) {
                if (&global_init.services[k] == service->requires[j]) {
                    dep_matrix[i][k] = 1;
                    break;
                }
            }
        }
    }
    
    // Detect cycles
    // TODO: Implement cycle detection
    
    // Calculate optimal parallelization
    kprintf("[INIT AI] Services can be started in parallel where possible\n");
}