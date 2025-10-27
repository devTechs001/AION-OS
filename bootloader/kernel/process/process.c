// AION OS Process Management with AI Scheduling
#include "process.h"
#include "../memory/memory.h"
#include "../ai/predictor.h"

// Process table
process_t process_table[MAX_PROCESSES];
uint32_t next_pid = 1;
process_t *current_process = NULL;
ready_queue_t ready_queue;

// AI scheduler
static ai_scheduler_t *ai_scheduler;

// Initialize process management
void process_init(void) {
    kprintf("[PROCESS] Initializing process management...\n");
    
    // Clear process table
    memset(process_table, 0, sizeof(process_table));
    
    // Initialize ready queue
    ready_queue.head = NULL;
    ready_queue.tail = NULL;
    ready_queue.count = 0;
    
    // Initialize AI scheduler
    ai_scheduler = ai_scheduler_create();
    
    // Create idle process
    create_idle_process();
    
    // Create kernel threads
    create_kernel_threads();
    
    kprintf("[PROCESS] Process management initialized\n");
}

// Create a new process with AI optimization
process_t* process_create(const char *name, void (*entry)(void), 
                          uint32_t priority) {
    // Find free slot in process table
    process_t *proc = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_UNUSED) {
            proc = &process_table[i];
            break;
        }
    }
    
    if (!proc) {
        kprintf("[PROCESS] Process table full!\n");
        return NULL;
    }
    
    // Initialize process structure
    proc->pid = next_pid++;
    strncpy(proc->name, name, PROCESS_NAME_MAX);
    proc->state = PROCESS_STATE_READY;
    proc->priority = priority;
    proc->quantum = DEFAULT_QUANTUM;
    
    // Use AI to predict resource requirements
    resource_prediction_t prediction = ai_scheduler->predict_resources(name);
    
    // Allocate memory based on prediction
    proc->memory.page_directory = pmm_alloc_pages(1);
    proc->memory.heap_size = prediction.heap_size;
    proc->memory.stack_size = prediction.stack_size;
    
    // Allocate stack
    proc->stack = pmm_alloc_pages(proc->memory.stack_size / PAGE_SIZE);
    proc->context.rsp = (uint64_t)proc->stack + proc->memory.stack_size;
    proc->context.rbp = proc->context.rsp;
    
    // Set entry point
    proc->context.rip = (uint64_t)entry;
    
    // Set up initial context
    proc->context.rflags = 0x202;  // Interrupts enabled
    proc->context.cs = USER_CS;
    proc->context.ss = USER_DS;
    
    // Initialize statistics
    proc->stats.cpu_time = 0;
    proc->stats.start_time = get_system_time();
    proc->stats.context_switches = 0;
    
    // Add to ready queue
    add_to_ready_queue(proc);
    
    // Train AI scheduler
    ai_scheduler->record_process_creation(proc);
    
    kprintf("[PROCESS] Created process %d: %s\n", proc->pid, proc->name);
    return proc;
}

// AI-powered scheduler
void schedule(void) {
    // Get AI scheduling decision
    scheduling_decision_t decision = ai_scheduler->make_decision(
        &ready_queue, current_process);
    
    // Apply scheduling decision
    switch (decision.action) {
        case SCHEDULE_SWITCH:
            // Context switch to new process
            if (decision.next_process) {
                switch_to_process(decision.next_process);
            }
            break;
            
        case SCHEDULE_CONTINUE:
            // Continue running current process
            if (current_process) {
                current_process->quantum = decision.quantum;
            }
            break;
            
        case SCHEDULE_IDLE:
            // Switch to idle process
            switch_to_idle();
            break;
            
        case SCHEDULE_MIGRATE:
            // Migrate process to different CPU core (for SMP)
            migrate_process(decision.next_process, decision.target_cpu);
            break;
    }
    
    // Update scheduler statistics
    ai_scheduler->update_statistics();
}

// Context switch implementation
void switch_to_process(process_t *next) {
    if (current_process == next) {
        return;  // Already running
    }
    
    process_t *prev = current_process;
    current_process = next;
    
    // Update process states
    if (prev && prev->state == PROCESS_STATE_RUNNING) {
        prev->state = PROCESS_STATE_READY;
        add_to_ready_queue(prev);
    }
    
    next->state = PROCESS_STATE_RUNNING;
    remove_from_ready_queue(next);
    
    // Update statistics
    if (prev) {
        prev->stats.context_switches++;
    }
    next->stats.context_switches++;
    
    // Perform context switch
    context_switch(&prev->context, &next->context);
}

// Context switch assembly implementation
__attribute__((naked)) void context_switch(context_t *old_ctx, context_t *new_ctx) {
    asm volatile(
        // Save old context
        "pushq %%rbp\n"
        "pushq %%rbx\n"
        "pushq %%r12\n"
        "pushq %%r13\n"
        "pushq %%r14\n"
        "pushq %%r15\n"
        "movq %%rsp, (%%rdi)\n"  // Save RSP to old context
        
        // Load new context
        "movq (%%rsi), %%rsp\n"  // Load RSP from new context
        "popq %%r15\n"
        "popq %%r14\n"
        "popq %%r13\n"
        "popq %%r12\n"
        "popq %%rbx\n"
        "popq %%rbp\n"
        "ret\n"
        :
        : "D"(old_ctx), "S"(new_ctx)
        : "memory"
    );
}

// Create idle process
void create_idle_process(void) {
    process_t *idle = process_create("idle", idle_process_entry, 0);
    idle->flags |= PROCESS_FLAG_SYSTEM;
}

// Idle process entry point
void idle_process_entry(void) {
    while (1) {
        // AI-powered CPU power management
        ai_scheduler->optimize_idle_state();
        
        // Enter low power state
        asm volatile("hlt");
    }
}

// Process termination
void process_exit(int exit_code) {
    current_process->state = PROCESS_STATE_ZOMBIE;
    current_process->exit_code = exit_code;
    
    // Free process resources
    pmm_free_pages(current_process->stack, 
                   current_process->memory.stack_size / PAGE_SIZE);
    pmm_free_pages(current_process->memory.page_directory, 1);
    
    // Notify AI scheduler
    ai_scheduler->record_process_exit(current_process);
    
    // Wake up parent if waiting
    if (current_process->parent) {
        wake_up_process(current_process->parent);
    }
    
    // Schedule next process
    schedule();
}