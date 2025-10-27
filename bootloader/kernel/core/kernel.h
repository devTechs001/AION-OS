#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Kernel constants
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS 0x18
#define USER_DS 0x20
#define TSS_SEGMENT 0x28

#define GDT_ENTRIES 6
#define IDT_ENTRIES 256

// Kernel state structure
typedef struct {
    bool running;
    bool panic;
    bool debug_mode;
    uint64_t ticks;
    uint64_t uptime;
} kernel_state_t;

// CPU features structure
typedef struct {
    char vendor[13];
    bool has_sse;
    bool has_sse2;
    bool has_sse3;
    bool has_avx;
    bool has_avx2;
    bool has_aes;
} cpu_features_t;

// GDT entry structure
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t flags_limit_high;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

// GDT pointer structure
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

// IDT entry structure
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

// TSS structure
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

// Function prototypes
void kernel_main(void);
void kernel_early_init(void);
void kernel_idle_loop(void);
void kernel_panic(const char *message);
void detect_cpu_features(void);
void init_gdt(void);
void init_idt(void);
void init_serial(void);
void kprintf(const char *format, ...);

// External functions
extern void load_gdt(gdt_ptr_t *gdt_ptr);
extern void load_idt(idt_ptr_t *idt_ptr);
extern void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, 
                  uint32_t *ecx, uint32_t *edx);

#endif // KERNEL_H