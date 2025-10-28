#ifndef INTEL_I915_H
#define INTEL_I915_H

#include <stdint.h>

// Intel Graphics Registers
#define INTEL_GFX_VGA_CONTROL   0x41000
#define INTEL_GFX_CPU_VGACNTRL  0x41000

// Display Pipeline Registers
#define INTEL_PIPE_A_CONF       0x70008
#define INTEL_PIPE_B_CONF       0x71008
#define INTEL_PIPE_C_CONF       0x72008

// Frame Buffer Control
#define INTEL_DSPCNTR_A         0x70180
#define INTEL_DSPADDR_A         0x70184
#define INTEL_DSPSTRIDE_A       0x70188
#define INTEL_DSPSURF_A         0x7019C

// Intel GPU Context
typedef struct {
    volatile uint8_t* mmio;
    uintptr_t gtt_base;
    size_t gtt_size;
    
    uintptr_t framebuffer_phys;
    void* framebuffer_virt;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_stride;
    uint32_t fb_bpp;
    
    // AI-Enhanced Rendering
    struct {
        uint64_t frames_rendered;
        uint32_t avg_frame_time_us;
        float predicted_fps;
        bool vsync_enabled;
    } ai_stats;
    
    spinlock_t lock;
} intel_gpu_t;

// Function Prototypes
void intel_i915_init(void);
int intel_i915_set_mode(intel_gpu_t* gpu, uint32_t width, uint32_t height, uint32_t bpp);
void intel_i915_flip_buffer(intel_gpu_t* gpu, uintptr_t buffer_phys);

// AI-Enhanced Features
void intel_i915_ai_optimize_refresh(intel_gpu_t* gpu);
void intel_i915_ai_predict_vsync(intel_gpu_t* gpu);

#endif // INTEL_I915_H

