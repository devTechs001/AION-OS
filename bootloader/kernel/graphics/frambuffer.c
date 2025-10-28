// AION OS Advanced Framebuffer with GPU Acceleration
#include "framebuffer.h"
#include "../drivers/pci.h"
#include "../memory/memory.h"
#include "../ai/predictor.h"

// Framebuffer info
static framebuffer_info_t fb_info;
static uint32_t *fb_backbuffer = NULL;
static bool double_buffering = true;

// AI graphics optimizer
static ai_graphics_optimizer_t *gfx_optimizer;

// 2D acceleration structures
typedef struct {
    bool available;
    void (*blit)(uint32_t *src, uint32_t *dst, int w, int h);
    void (*fill_rect)(uint32_t color, int x, int y, int w, int h);
    void (*draw_line)(uint32_t color, int x1, int y1, int x2, int y2);
    void (*draw_circle)(uint32_t color, int x, int y, int r);
} hw_accel_t;

static hw_accel_t hw_accel;

// Initialize framebuffer
void framebuffer_init(multiboot_info_t *mboot_info) {
    kprintf("[FRAMEBUFFER] Initializing graphics...\n");
    
    // Get framebuffer info from multiboot
    if (mboot_info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        fb_info.address = (void*)mboot_info->framebuffer_addr;
        fb_info.width = mboot_info->framebuffer_width;
        fb_info.height = mboot_info->framebuffer_height;
        fb_info.pitch = mboot_info->framebuffer_pitch;
        fb_info.bpp = mboot_info->framebuffer_bpp;
        fb_info.type = mboot_info->framebuffer_type;
    } else {
        // Try VESA modes
        if (!init_vesa_framebuffer()) {
            kprintf("[FRAMEBUFFER] No framebuffer available\n");
            return;
        }
    }
    
    kprintf("[FRAMEBUFFER] %dx%d @ %d bpp\n", 
            fb_info.width, fb_info.height, fb_info.bpp);
    kprintf("[FRAMEBUFFER] Address: 0x%llx, Pitch: %d\n",
            (uint64_t)fb_info.address, fb_info.pitch);
    
    // Allocate backbuffer for double buffering
    size_t fb_size = fb_info.pitch * fb_info.height;
    fb_backbuffer = (uint32_t*)kmalloc(fb_size);
    
    if (!fb_backbuffer) {
        double_buffering = false;
        kprintf("[FRAMEBUFFER] No double buffering (insufficient memory)\n");
    }
    
    // Initialize AI graphics optimizer
    gfx_optimizer = ai_graphics_optimizer_create();
    
    // Detect and initialize hardware acceleration
    init_hw_acceleration();
    
    // Clear screen
    framebuffer_clear(0x000000);
    
    kprintf("[FRAMEBUFFER] Framebuffer initialized\n");
}

// Initialize hardware acceleration
void init_hw_acceleration(void) {
    memset(&hw_accel, 0, sizeof(hw_accel));
    
    // Detect GPU
    pci_device_t *gpu = pci_find_device(PCI_CLASS_DISPLAY, PCI_SUBCLASS_VGA);
    
    if (gpu) {
        kprintf("[FRAMEBUFFER] GPU detected: %04x:%04x\n", 
                gpu->vendor_id, gpu->device_id);
        
        // Initialize GPU-specific acceleration
        if (gpu->vendor_id == PCI_VENDOR_INTEL) {
            init_intel_gpu_accel(gpu);
        } else if (gpu->vendor_id == PCI_VENDOR_NVIDIA) {
            init_nvidia_gpu_accel(gpu);
        } else if (gpu->vendor_id == PCI_VENDOR_AMD) {
            init_amd_gpu_accel(gpu);
        }
    }
    
    // Fallback to software rendering if no hardware accel
    if (!hw_accel.available) {
        kprintf("[FRAMEBUFFER] Using software rendering\n");
        init_software_rendering();
    }
}

// Put pixel (with bounds checking)
void framebuffer_put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= fb_info.width || y < 0 || y >= fb_info.height) {
        return;
    }
    
    uint32_t *buffer = double_buffering ? fb_backbuffer : 
                       (uint32_t*)fb_info.address;
    buffer[y * (fb_info.pitch / 4) + x] = color;
}

// Draw rectangle
void framebuffer_draw_rect(int x, int y, int w, int h, uint32_t color) {
    // Use hardware acceleration if available
    if (hw_accel.available && hw_accel.fill_rect) {
        hw_accel.fill_rect(color, x, y, w, h);
        return;
    }
    
    // Software fallback with SIMD optimization
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            framebuffer_put_pixel(x + dx, y + dy, color);
        }
    }
}

// Draw filled rectangle (optimized)
void framebuffer_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (hw_accel.available && hw_accel.fill_rect) {
        hw_accel.fill_rect(color, x, y, w, h);
        return;
    }
    
    // Clipping
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > fb_info.width) w = fb_info.width - x;
    if (y + h > fb_info.height) h = fb_info.height - y;
    
    if (w <= 0 || h <= 0) return;
    
    uint32_t *buffer = double_buffering ? fb_backbuffer : 
                       (uint32_t*)fb_info.address;
    
    int pitch_pixels = fb_info.pitch / 4;
    
    // Vectorized fill using SSE
    #ifdef __SSE2__
    __m128i color_vec = _mm_set1_epi32(color);
    
    for (int dy = 0; dy < h; dy++) {
        uint32_t *row = &buffer[(y + dy) * pitch_pixels + x];
        int dx = 0;
        
        // Fill 4 pixels at a time
        for (; dx + 4 <= w; dx += 4) {
            _mm_storeu_si128((__m128i*)&row[dx], color_vec);
        }
        
        // Fill remaining pixels
        for (; dx < w; dx++) {
            row[dx] = color;
        }
    }
    #else
    // Scalar fallback
    for (int dy = 0; dy < h; dy++) {
        uint32_t *row = &buffer[(y + dy) * pitch_pixels + x];
        for (int dx = 0; dx < w; dx++) {
            row[dx] = color;
        }
    }
    #endif
}

// Draw line (Bresenham's algorithm)
void framebuffer_draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    if (hw_accel.available && hw_accel.draw_line) {
        hw_accel.draw_line(color, x1, y1, x2, y2);
        return;
    }
    
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        framebuffer_put_pixel(x1, y1, color);
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// Draw circle (Midpoint circle algorithm)
void framebuffer_draw_circle(int cx, int cy, int radius, uint32_t color) {
    if (hw_accel.available && hw_accel.draw_circle) {
        hw_accel.draw_circle(color, cx, cy, radius);
        return;
    }
    
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        framebuffer_put_pixel(cx + x, cy + y, color);
        framebuffer_put_pixel(cx + y, cy + x, color);
        framebuffer_put_pixel(cx - y, cy + x, color);
        framebuffer_put_pixel(cx - x, cy + y, color);
        framebuffer_put_pixel(cx - x, cy - y, color);
        framebuffer_put_pixel(cx - y, cy - x, color);
        framebuffer_put_pixel(cx + y, cy - x, color);
        framebuffer_put_pixel(cx + x, cy - y, color);
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

// Blit image
void framebuffer_blit(uint32_t *src, int src_x, int src_y, 
                     int dst_x, int dst_y, int w, int h, int src_pitch) {
    if (hw_accel.available && hw_accel.blit) {
        // Hardware accelerated blit
        uint32_t *src_ptr = &src[src_y * (src_pitch / 4) + src_x];
        uint32_t *dst_ptr = double_buffering ? fb_backbuffer : 
                           (uint32_t*)fb_info.address;
        hw_accel.blit(src_ptr, dst_ptr, w, h);
        return;
    }
    
    // Software blit with alpha blending support
    uint32_t *buffer = double_buffering ? fb_backbuffer : 
                       (uint32_t*)fb_info.address;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src_idx = (src_y + y) * (src_pitch / 4) + (src_x + x);
            int dst_idx = (dst_y + y) * (fb_info.pitch / 4) + (dst_x + x);
            
            uint32_t src_pixel = src[src_idx];
            uint32_t alpha = (src_pixel >> 24) & 0xFF;
            
            if (alpha == 255) {
                // Opaque
                buffer[dst_idx] = src_pixel;
            } else if (alpha > 0) {
                // Alpha blending
                uint32_t dst_pixel = buffer[dst_idx];
                buffer[dst_idx] = blend_pixels(src_pixel, dst_pixel, alpha);
            }
        }
    }
}

// Alpha blending
static inline uint32_t blend_pixels(uint32_t src, uint32_t dst, uint8_t alpha) {
    uint8_t inv_alpha = 255 - alpha;
    
    uint8_t r = ((src >> 16 & 0xFF) * alpha + (dst >> 16 & 0xFF) * inv_alpha) / 255;
    uint8_t g = ((src >> 8 & 0xFF) * alpha + (dst >> 8 & 0xFF) * inv_alpha) / 255;
    uint8_t b = ((src & 0xFF) * alpha + (dst & 0xFF) * inv_alpha) / 255;
    
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

// Swap buffers (double buffering)
void framebuffer_swap_buffers(void) {
    if (!double_buffering) return;
    
    // AI decision: Should we use vsync?
    if (gfx_optimizer->should_use_vsync()) {
        wait_for_vsync();
    }
    
    // Copy backbuffer to framebuffer
    memcpy(fb_info.address, fb_backbuffer, 
           fb_info.pitch * fb_info.height);
}

// Clear framebuffer
void framebuffer_clear(uint32_t color) {
    framebuffer_fill_rect(0, 0, fb_info.width, fb_info.height, color);
}

// Draw text (using built-in font)
void framebuffer_draw_text(const char *text, int x, int y, uint32_t color) {
    extern const uint8_t font_8x16[256][16];
    
    int char_x = x;
    int char_y = y;
    
    while (*text) {
        if (*text == '\n') {
            char_y += 16;
            char_x = x;
            text++;
            continue;
        }
        
        // Draw character
        const uint8_t *glyph = font_8x16[(uint8_t)*text];
        
        for (int dy = 0; dy < 16; dy++) {
            uint8_t row = glyph[dy];
            for (int dx = 0; dx < 8; dx++) {
                if (row & (1 << (7 - dx))) {
                    framebuffer_put_pixel(char_x + dx, char_y + dy, color);
                }
            }
        }
        
        char_x += 8;
        text++;
    }
}