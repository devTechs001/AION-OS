// AION OS Kernel Core
#include "kernel.h"
#include "../memory/memory.h"
#include "../process/process.h"
#include "../drivers/driver.h"
#include "../terminal/terminal.h"
#include "../ai/predictor.h"

// Kernel version info
#define KERNEL_VERSION "1.0.0"
#define KERNEL_NAME "AION"

// Global kernel state
kernel_state_t kernel_state = {
    .running = false,
    .panic = false,
    .debug_mode = true
};

// Multiboot info structure
extern multiboot_info_t *multiboot_info;

// Early kernel initialization
void kernel_early_init(void) {
    // Disable interrupts during init
    asm volatile("cli");
    
    // Initialize serial port for debugging
    init_serial();
    kprintf("[KERNEL] AION OS %s starting...\n", KERNEL_VERSION);
    
    // Initialize CPU features detection
    detect_cpu_features();
    
    // Setup GDT and IDT
    init_gdt();
    init_idt();
    
    kprintf("[KERNEL] CPU initialized\n");
}

// Main kernel entry point
void kernel_main(void) {
    // Early initialization
    kernel_early_init();
    
    // Initialize memory management
    kprintf("[KERNEL] Initializing memory management...\n");
    memory_init(multiboot_info);
    heap_init();
    
    // Initialize AI predictor early for optimization
    kprintf("[KERNEL] Initializing AI predictor...\n");
    ai_predictor_init();
    
    // Initialize process management
    kprintf("[KERNEL] Initializing process management...\n");
    process_init();
    scheduler_init();
    
    // Initialize drivers
    kprintf("[KERNEL] Initializing drivers...\n");
    driver_manager_init();
    pci_init();
    
    // Initialize filesystem
    kprintf("[KERNEL] Initializing filesystem...\n");
    vfs_init();
    
    // Initialize terminal and shell
    kprintf("[KERNEL] Initializing terminal...\n");
    terminal_init();
    
    // Enable interrupts
    asm volatile("sti");
    
    // Mark kernel as running
    kernel_state.running = true;
    
    // Print welcome message
    terminal_clear();
    terminal_print_banner();
    
    // Start the init process
    kprintf("[KERNEL] Starting init process...\n");
    start_init_process();
    
    // Enter kernel idle loop
    kernel_idle_loop();
}

// Kernel idle loop with AI optimization
void kernel_idle_loop(void) {
    while (kernel_state.running) {
        // AI-powered CPU optimization
        ai_optimize_cpu_state();
        
        // Schedule next process
        schedule();
        
        // Handle pending interrupts
        handle_pending_interrupts();
        
        // Power saving mode
        if (can_enter_low_power()) {
            asm volatile("hlt");
        }
    }
}

// Kernel panic handler
void kernel_panic(const char *message) {
    kernel_state.panic = true;
    
    // Disable interrupts
    asm volatile("cli");
    
    // Print panic message
    terminal_set_color(TERMINAL_COLOR_RED);
    kprintf("\n\n[KERNEL PANIC] %s\n", message);
    kprintf("System halted. Please restart your computer.\n");
    
    // Dump registers and stack trace
    dump_registers();
    dump_stack_trace();
    
    // Halt the system
    while (1) {
        asm volatile("hlt");
    }
}

// CPU feature detection
void detect_cpu_features(void) {
    cpu_features_t features = {0};
    uint32_t eax, ebx, ecx, edx;
    
    // Get CPU vendor
    cpuid(0, &eax, &ebx, &ecx, &edx);
    *((uint32_t*)&features.vendor[0]) = ebx;
    *((uint32_t*)&features.vendor[4]) = edx;
    *((uint32_t*)&features.vendor[8]) = ecx;
    
    // Check for extended features
    cpuid(1, &eax, &ebx, &ecx, &edx);
    features.has_sse = (edx >> 25) & 1;
    features.has_sse2 = (edx >> 26) & 1;
    features.has_sse3 = ecx & 1;
    features.has_avx = (ecx >> 28) & 1;
    
    kprintf("[CPU] Vendor: %s\n", features.vendor);
    kprintf("[CPU] Features: SSE=%d SSE2=%d SSE3=%d AVX=%d\n",
            features.has_sse, features.has_sse2, 
            features.has_sse3, features.has_avx);
}

// Initialize GDT (Global Descriptor Table)
void init_gdt(void) {
    gdt_entry_t gdt[GDT_ENTRIES];
    gdt_ptr_t gdt_ptr;
    
    // Null segment
    set_gdt_entry(&gdt[0], 0, 0, 0, 0);
    
    // Kernel code segment
    set_gdt_entry(&gdt[1], 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // Kernel data segment
    set_gdt_entry(&gdt[2], 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // User code segment
    set_gdt_entry(&gdt[3], 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // User data segment
    set_gdt_entry(&gdt[4], 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // TSS segment (for task switching)
    set_gdt_entry(&gdt[5], (uint32_t)&tss, sizeof(tss_t), 0x89, 0x00);
    
    // Load GDT
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;
    load_gdt(&gdt_ptr);
}

// Initialize IDT (Interrupt Descriptor Table)
void init_idt(void) {
    idt_entry_t idt[IDT_ENTRIES];
    idt_ptr_t idt_ptr;
    
    // Clear IDT
    memset(idt, 0, sizeof(idt));
    
    // Set up exception handlers (0-31)
    for (int i = 0; i < 32; i++) {
        set_idt_entry(&idt[i], (uint64_t)exception_handlers[i], 
                     KERNEL_CS, IDT_FLAG_PRESENT | IDT_FLAG_RING0);
    }
    
    // Set up interrupt handlers (32-255)
    for (int i = 32; i < IDT_ENTRIES; i++) {
        set_idt_entry(&idt[i], (uint64_t)interrupt_handlers[i-32],
                     KERNEL_CS, IDT_FLAG_PRESENT | IDT_FLAG_RING0);
    }
    
    // Load IDT
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;
    load_idt(&idt_ptr);
}