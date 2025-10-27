// AION OS Memory Management with AI Prediction
#include "memory.h"
#include "../ai/predictor.h"

// Physical memory bitmap
static uint32_t *memory_bitmap;
static uint32_t total_memory;
static uint32_t used_memory;
static uint32_t free_memory;

// Memory zones for NUMA support
memory_zone_t memory_zones[MAX_MEMORY_ZONES];
uint32_t num_memory_zones = 0;

// AI memory predictor
static ai_memory_predictor_t *mem_predictor;

// Initialize memory management
void memory_init(multiboot_info_t *mboot_info) {
    kprintf("[MEMORY] Initializing memory management...\n");
    
    // Get total memory from multiboot
    total_memory = mboot_info->mem_upper * 1024;
    free_memory = total_memory;
    used_memory = 0;
    
    kprintf("[MEMORY] Total memory: %d MB\n", total_memory / (1024 * 1024));
    
    // Initialize memory bitmap
    uint32_t bitmap_size = total_memory / (PAGE_SIZE * 32);
    memory_bitmap = (uint32_t*)MEMORY_BITMAP_ADDR;
    memset(memory_bitmap, 0, bitmap_size);
    
    // Mark kernel memory as used
    mark_memory_used(0, KERNEL_SIZE);
    
    // Initialize memory zones
    init_memory_zones(mboot_info);
    
    // Initialize AI memory predictor
    mem_predictor = ai_memory_predictor_create();
    
    kprintf("[MEMORY] Memory management initialized\n");
}

// Allocate physical pages with AI prediction
void* pmm_alloc_pages(size_t num_pages) {
    // Use AI to predict best allocation strategy
    allocation_hint_t hint = mem_predictor->predict_allocation(num_pages);
    
    // Find contiguous free pages
    uint32_t start_page = find_free_pages(num_pages, hint);
    if (start_page == INVALID_PAGE) {
        // Try memory compaction
        if (compact_memory()) {
            start_page = find_free_pages(num_pages, hint);
        }
        
        if (start_page == INVALID_PAGE) {
            kernel_panic("Out of physical memory!");
            return NULL;
        }
    }
    
    // Mark pages as used
    for (size_t i = 0; i < num_pages; i++) {
        set_page_used(start_page + i);
    }
    
    // Update statistics
    used_memory += num_pages * PAGE_SIZE;
    free_memory -= num_pages * PAGE_SIZE;
    
    // Train AI predictor
    mem_predictor->record_allocation(num_pages, start_page);
    
    return (void*)(start_page * PAGE_SIZE);
}

// Free physical pages
void pmm_free_pages(void *addr, size_t num_pages) {
    uint32_t start_page = (uint32_t)addr / PAGE_SIZE;
    
    // Mark pages as free
    for (size_t i = 0; i < num_pages; i++) {
        set_page_free(start_page + i);
    }
    
    // Update statistics
    used_memory -= num_pages * PAGE_SIZE;
    free_memory += num_pages * PAGE_SIZE;
    
    // Notify AI predictor
    mem_predictor->record_free(num_pages, start_page);
    
    // Attempt to coalesce free blocks
    coalesce_free_blocks(start_page, num_pages);
}

// Initialize memory zones for NUMA support
void init_memory_zones(multiboot_info_t *mboot_info) {
    // Parse memory map from multiboot
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)mboot_info->mmap_addr;
    
    while ((uint32_t)mmap < mboot_info->mmap_addr + mboot_info->mmap_length) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            // Create memory zone
            memory_zone_t *zone = &memory_zones[num_memory_zones++];
            zone->start_addr = mmap->addr;
            zone->end_addr = mmap->addr + mmap->len;
            zone->size = mmap->len;
            zone->free_pages = mmap->len / PAGE_SIZE;
            zone->used_pages = 0;
            
            kprintf("[MEMORY] Zone %d: 0x%llx - 0x%llx (%lld MB)\n",
                   num_memory_zones - 1, zone->start_addr, 
                   zone->end_addr, zone->size / (1024 * 1024));
        }
        
        mmap = (multiboot_memory_map_t*)((uint32_t)mmap + 
                mmap->size + sizeof(mmap->size));
    }
}

// Memory compaction with AI optimization
bool compact_memory(void) {
    kprintf("[MEMORY] Starting AI-optimized memory compaction...\n");
    
    // Use AI to determine best compaction strategy
    compaction_strategy_t strategy = mem_predictor->get_compaction_strategy();
    
    uint32_t pages_moved = 0;
    
    switch (strategy) {
        case COMPACT_AGGRESSIVE:
            pages_moved = compact_aggressive();
            break;
        case COMPACT_LAZY:
            pages_moved = compact_lazy();
            break;
        case COMPACT_SMART:
            pages_moved = compact_smart();
            break;
    }
    
    kprintf("[MEMORY] Compaction complete, moved %d pages\n", pages_moved);
    return pages_moved > 0;
}

// AI-powered smart memory compaction
uint32_t compact_smart(void) {
    uint32_t pages_moved = 0;
    
    // Analyze memory fragmentation
    fragmentation_info_t frag_info = analyze_fragmentation();
    
    // Use AI to predict which pages to move
    page_move_list_t *moves = mem_predictor->predict_moves(frag_info);
    
    // Execute moves
    for (size_t i = 0; i < moves->count; i++) {
        page_move_t *move = &moves->moves[i];
        if (move_page(move->from, move->to)) {
            pages_moved++;
        }
    }
    
    free(moves);
    return pages_moved;
}