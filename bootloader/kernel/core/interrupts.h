#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <stdbool.h>

// IDT flags
#define IDT_INTERRUPT_GATE 0x8E
#define IDT_TRAP_GATE 0x8F
#define IDT_PRESENT 0x80

// IRQ numbers
#define IRQ_TIMER 0
#define IRQ_KEYBOARD 1
#define IRQ_CASCADE 2
#define IRQ_COM2 3
#define IRQ_COM1 4
#define IRQ_LPT2 5
#define IRQ_FLOPPY 6
#define IRQ_LPT1 7
#define IRQ_RTC 8
#define IRQ_MOUSE 12
#define IRQ_FPU 13
#define IRQ_PRIMARY_ATA 14
#define IRQ_SECONDARY_ATA 15

// System call numbers
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_FORK 4
#define SYS_EXEC 5
#define SYS_EXIT 6
#define SYS_GETPID 7
#define SYS_MMAP 8
#define SYS_MUNMAP 9

// IDT entry structure
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

// Interrupt frame structure
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_num, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) interrupt_frame_t;

// Interrupt statistics
typedef struct {
    uint64_t count;
    uint64_t last_time;
    uint64_t total_time;
    uint64_t min_time;
    uint64_t max_time;
} interrupt_stats_t;

// Interrupt handler type
typedef void (*interrupt_handler_t)(interrupt_frame_t *frame);

// Function prototypes
void interrupts_init(void);
void set_idt_gate(uint8_t num, uint64_t handler, uint16_t selector,
                  uint8_t flags, uint8_t dpl);
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler);
void interrupt_dispatcher(interrupt_frame_t *frame);
void dump_interrupt_frame(interrupt_frame_t *frame);
void send_eoi(uint8_t irq);

#endif // INTERRUPTS_H