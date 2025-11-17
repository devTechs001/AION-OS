/*
 * AION OS - Kernel Main Entry Point
 * The core kernel initialization and main loop
 */

#include <aion/kernel.h>
#include <aion/types.h>

// Main kernel entry point
int kernel_main(void) {
    // Initialize core kernel subsystems
    if (!init_kernel_subsystems()) {
        return -1;
    }
    
    // Initialize AI systems
    if (!init_ai_systems()) {
        return -1;
    }
    
    // Main kernel loop
    while (1) {
        // Process interrupts and system calls
        handle_pending_interrupts();
        process_system_calls();
    }
    
    return 0;
}

static bool init_kernel_subsystems(void) {
    // Initialize each kernel subsystem
    if (!init_memory_manager()) {
        return false;
    }
    
    if (!init_scheduler()) {
        return false;
    }
    
    if (!init_process_manager()) {
        return false;
    }
    
    if (!init_filesystem()) {
        return false;
    }
    
    return true;
}

static bool init_ai_systems(void) {
    // Initialize AI core services
    if (!init_ai_engine()) {
        return false;
    }
    
    if (!init_model_manager()) {
        return false;
    }
    
    return true;
}