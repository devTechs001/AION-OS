#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Memory constants
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define KERNEL_SIZE 0x400000  // 4MB for kernel
#define MEMORY_BITMAP_ADDR 0x500000
#define HEAP_START 0x1000000
#define HEAP_SIZE 0x1000000  // 16MB initial heap
#define MAX_MEMORY_ZONES 16
#define INVALID_PAGE 0xFFFFFFFF

// Memory zone structure
typedef struct {
    uint64_t start_addr;
    uint64_t end_addr;
    uint64_t size;
    uint32_t free_pages;
    uint32_t used_pages;
    uint32_t flags;
} memory_zone_t;

// Allocation hint from AI
typedef struct {
    uint32_t preferred_zone;
    uint32_t alignment;
    bool contiguous;
    bool cache_line_aligned;
} allocation_hint_t;

// Compaction strategies
typedef enum {
    COMPACT_AGGRESSIVE,
    COMPACT_LAZY,
    COMPACT_SMART
} compaction_strategy_t;

// Fragmentation info
typedef struct {
    uint32_t total_fragments;
    uint32_t largest_free_block;
    float fragmentation_ratio;
} fragmentation_info_t;

// Page move structure
typedef struct {
    uint32_t from;
    uint32_t to;
} page_move_t;

typedef struct {
    page_move_t *moves;
    size_t count;
} page_move_list_t;

// Function prototypes
void memory_init(multiboot_info_t *mboot_info);
void* pmm_alloc_pages(size_t num_pages);
void pmm_free_pages(void *addr, size_t num_pages);
void init_memory_zones(multiboot_info_t *mboot_info);
bool compact_memory(void);
uint32_t compact_smart(void);
fragmentation_info_t analyze_fragmentation(void);

// Inline functions for bitmap operations
static inline void set_page_used(uint32_t page) {
    memory_bitmap[page / 32] |= (1 << (page % 32));
}

static inline void set_page_free(uint32_t page) {
    memory_bitmap[page / 32] &= ~(1 << (page % 32));
}

static inline bool is_page_free(uint32_t page) {
    return !(memory_bitmap[page / 32] & (1 << (page % 32)));
}

#endif // MEMORY_H