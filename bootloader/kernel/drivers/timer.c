// AION OS High-Precision Timer with AI Optimization
#include "timer.h"
#include "../core/interrupts.h"
#include "../ai/predictor.h"

// Timer sources
static timer_source_t timer_source = TIMER_PIT;
static uint64_t system_ticks = 0;
static uint64_t system_time_ms = 0;

// Timer callbacks for AI scheduling
#define MAX_TIMER_CALLBACKS 256
static timer_callback_t timer_callbacks[MAX_TIMER_CALLBACKS];
static uint32_t num_callbacks = 0;

// AI timer optimizer
static ai_timer_optimizer_t *timer_optimizer;

// Initialize timer system
void timer_init(void) {
    kprintf("[TIMER] Initializing timer system...\n");
    
    // Detect available timer sources
    if (detect_hpet()) {
        timer_source = TIMER_HPET;
        init_hpet();
        kprintf("[TIMER] Using HPET\n");
    } else if (detect_tsc()) {
        timer_source = TIMER_TSC;
        init_tsc();
        kprintf("[TIMER] Using TSC\n");
    } else {
        timer_source = TIMER_PIT;
        init_pit();
        kprintf("[TIMER] Using PIT\n");
    }
    
    // Initialize AI timer optimizer
    timer_optimizer = ai_timer_optimizer_create();
    
    // Register timer interrupt handler
    register_interrupt_handler(IRQ_TIMER + 32, timer_interrupt_handler);
    
    kprintf("[TIMER] Timer initialized, frequency: %d Hz\n", TIMER_FREQUENCY);
}

// Initialize PIT (Programmable Interval Timer)
void init_pit(void) {
    uint16_t divisor = PIT_FREQUENCY / TIMER_FREQUENCY;
    
    // Set PIT to mode 3 (square wave generator)
    outb(PIT_COMMAND_PORT, 0x36);
    
    // Set frequency divisor
    outb(PIT_CHANNEL0_PORT, divisor & 0xFF);
    outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xFF);
}

// Initialize HPET (High Precision Event Timer)
void init_hpet(void) {
    // Parse ACPI tables to find HPET base address
    hpet_t *hpet = (hpet_t*)find_hpet_base();
    
    if (!hpet) {
        kprintf("[TIMER] HPET not found, falling back to PIT\n");
        timer_source = TIMER_PIT;
        init_pit();
        return;
    }
    
    // Enable HPET
    uint64_t config = hpet->general_config;
    config |= HPET_ENABLE;
    hpet->general_config = config;
    
    // Configure timer 0
    hpet->timer[0].config = HPET_TIMER_PERIODIC | HPET_TIMER_INT_ENABLE;
    
    // Set period
    uint64_t period = (1000000000000000ULL / TIMER_FREQUENCY) / hpet->period;
    hpet->timer[0].comparator = period;
    hpet->main_counter = 0;
    
    kprintf("[TIMER] HPET initialized at 0x%llx\n", (uint64_t)hpet);
}

// Initialize TSC (Time Stamp Counter)
void init_tsc(void) {
    // Calibrate TSC frequency
    uint64_t tsc_start = read_tsc();
    pit_sleep(100);  // Sleep 100ms using PIT
    uint64_t tsc_end = read_tsc();
    
    uint64_t tsc_frequency = (tsc_end - tsc_start) * 10;  // Hz
    kprintf("[TIMER] TSC frequency: %llu MHz\n", tsc_frequency / 1000000);
    
    // Store TSC frequency for time calculations
    timer_optimizer->tsc_frequency = tsc_frequency;
}

// Read TSC
static inline uint64_t read_tsc(void) {
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

// Timer interrupt handler
void timer_interrupt_handler(interrupt_frame_t *frame) {
    system_ticks++;
    system_time_ms += (1000 / TIMER_FREQUENCY);
    
    // AI-optimized callback processing
    if (timer_optimizer->should_process_callbacks(system_ticks)) {
        process_timer_callbacks();
    }
    
    // Update scheduler quantum
    update_scheduler_quantum();
    
    // AI power management
    timer_optimizer->optimize_power_state();
}

// Register timer callback
uint32_t register_timer_callback(uint64_t interval_ms, 
                                  timer_callback_func_t callback,
                                  void *data, bool repeating) {
    if (num_callbacks >= MAX_TIMER_CALLBACKS) {
        return 0;
    }
    
    timer_callback_t *cb = &timer_callbacks[num_callbacks];
    cb->id = num_callbacks + 1;
    cb->interval_ms = interval_ms;
    cb->next_trigger = system_time_ms + interval_ms;
    cb->callback = callback;
    cb->data = data;
    cb->repeating = repeating;
    cb->enabled = true;
    
    num_callbacks++;
    
    // AI prediction: Optimize callback order
    timer_optimizer->optimize_callback_order(timer_callbacks, num_callbacks);
    
    return cb->id;
}

// Process timer callbacks
void process_timer_callbacks(void) {
    for (uint32_t i = 0; i < num_callbacks; i++) {
        timer_callback_t *cb = &timer_callbacks[i];
        
        if (!cb->enabled) continue;
        
        if (system_time_ms >= cb->next_trigger) {
            // Call callback
            cb->callback(cb->data);
            
            if (cb->repeating) {
                // Reschedule
                cb->next_trigger = system_time_ms + cb->interval_ms;
            } else {
                // Disable one-shot callback
                cb->enabled = false;
            }
        }
    }
}

// High-resolution sleep
void sleep_ms(uint64_t milliseconds) {
    uint64_t target = system_time_ms + milliseconds;
    
    // AI-optimized sleep strategy
    sleep_strategy_t strategy = timer_optimizer->get_sleep_strategy(milliseconds);
    
    switch (strategy) {
        case SLEEP_BUSY_WAIT:
            // For very short sleeps
            while (system_time_ms < target) {
                asm volatile("pause");
            }
            break;
            
        case SLEEP_YIELD:
            // For short sleeps
            while (system_time_ms < target) {
                yield_cpu();
            }
            break;
            
        case SLEEP_BLOCK:
            // For longer sleeps
            block_until(target);
            break;
    }
}

// Get current time in various formats
uint64_t get_system_time(void) {
    switch (timer_source) {
        case TIMER_TSC:
            return read_tsc() / (timer_optimizer->tsc_frequency / 1000000);
        case TIMER_HPET:
            return read_hpet_counter();
        case TIMER_PIT:
        default:
            return system_time_ms;
    }
}

// Get uptime
void get_uptime(uptime_t *uptime) {
    uint64_t total_seconds = system_time_ms / 1000;
    
    uptime->days = total_seconds / 86400;
    uptime->hours = (total_seconds % 86400) / 3600;
    uptime->minutes = (total_seconds % 3600) / 60;
    uptime->seconds = total_seconds % 60;
    uptime->milliseconds = system_time_ms % 1000;
}