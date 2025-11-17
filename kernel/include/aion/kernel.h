/*
 * AION OS - Kernel Main Header
 * Main kernel interface definitions
 */

#ifndef AION_KERNEL_H
#define AION_KERNEL_H

#include "types.h"

// Kernel initialization functions
int kernel_main(void);
bool init_kernel_subsystems(void);
bool init_ai_systems(void);

// Core kernel services
void handle_pending_interrupts(void);
void process_system_calls(void);

// Subsystem initialization
bool init_memory_manager(void);
bool init_scheduler(void);
bool init_process_manager(void);
bool init_filesystem(void);
bool init_ai_engine(void);
bool init_model_manager(void);

#endif