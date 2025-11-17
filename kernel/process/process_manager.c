/*
 * AION OS - Process Management
 * Advanced process and thread management
 */

#include <aion/process.h>
#include <aion/types.h>

// Initialize process management system
bool init_process_manager(void) {
    // Initialize process structures
    if (!init_process_structures()) {
        return false;
    }
    
    // Initialize thread management
    if (!init_thread_manager()) {
        return false;
    }
    
    // Initialize AI process optimization
    if (!init_ai_process_optimizer()) {
        return false;
    }
    
    return true;
}