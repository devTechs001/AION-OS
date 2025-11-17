/*
 * AION OS - Kernel Types and Definitions
 * Core type definitions for the kernel
 */

#ifndef AION_TYPES_H
#define AION_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Basic types
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// Common structures
typedef struct {
    uint64_t base;
    uint64_t size;
} memory_region_t;

typedef struct {
    uint32_t eax, ebx, ecx, edx;
} cpuid_info_t;

// Return codes
typedef enum {
    AION_SUCCESS = 0,
    AION_ERROR = -1,
    AION_INVALID_PARAM = -2,
    AION_OUT_OF_MEMORY = -3,
    AION_NOT_IMPLEMENTED = -4
} aion_status_t;

#endif