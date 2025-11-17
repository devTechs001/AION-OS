#include "ai_compositor.h"
#include "../../kernel/drivers/graphics/framebuffer.h"
#include "../../kernel/ai/nlp/nlp_engine.h"
#include <string.h>
#include <stdlib.h>

static wayland_compositor_t global_compositor = {0};

void compositor_init(void) {
    memset(&global_compositor, 0, sizeof(global_compositor));
    spinlock_init(&global_compositor.lock);
    
    kprintf("[Compositor] Initializing AI-powered Wayland compositor...\n");
    
    // Initialize framebuffer
    framebuffer_t* fb = framebuffer_get_primary();
    if (!fb) {
        kprintf("[Compositor] Error: No framebuffer available\n");
        return;
    }
    
    // Create primary output
    wayland_output_t* output = kmalloc(sizeof(wayland_output_t));
    memset(output, 0, sizeof(wayland_output_t));
    
    output->id = 0;
    strcpy(output->name, "HDMI-1");
    output->width = fb->width;
    output->height = fb->height;
    output->refresh_rate = 60;
    output->framebuffer = fb->buffer;
    output->fb_size = fb->pitch * fb->height;
    output->fb_stride = fb->pitch;
    output->connected = true;
    output->enabled = true;
    
    global_compositor.outputs[0] = output;
    global_compositor.num_outputs = 1;
    
    // Enable AI features
    global_compositor.ai.smart_window_placement = true;
    global_compositor.ai.auto_tiling = false;
    global_compositor.ai.gesture_prediction = true;
    global_compositor.ai.performance_optimization = true;
    
    // Initialize workspaces
    global_compositor.current_workspace = 0;
    global_compositor.workspaces[0].active = true;
    
    // Cursor
    global_compositor.cursor_x = output->width / 2;
    global_compositor.cursor_y = output->height / 2;
    global_compositor.cursor_visible = true;
    
    global_compositor.vsync_enabled = true;
    global_compositor.running = true;
    
    kprintf("[Compositor] Initialized %dx%d @ %d Hz\n",
            output->width, output->height, output->refresh_rate);
    kprintf("[Compositor] AI features enabled: Smart placement, Gesture prediction\n");
}

// Create new surface (window)
wayland_surface_t* compositor_create_surface(void) {
    spinlock_acquire(&global_compositor.lock);
    
    wayland_surface_t* surface = kmalloc(sizeof(wayland_surface_t));
    memset(surface, 0, sizeof(wayland_surface_t));
    
    surface->id = global_compositor.num_surfaces + 1;
    surface->mapped = false;
    surface->focused = false;
    
    // AI: Smart window placement
    if (global_compositor.ai.smart_window_placement) {
        compositor_ai_smart_placement(surface);
    } else {
        // Default placement (cascade)
        surface->x = global_compositor.num_surfaces * 30;
        surface->y = global_compositor.num_surfaces * 30;
    }
    
    // AI: Predict window size based on app type
    int predicted_width, predicted_height;
    compositor_ai_predict_window_size(surface, &predicted_width, &predicted_height);
    
    surface->width = predicted_width;
    surface->height = predicted_height;
    
    surface->ai.ai_resize_enabled = true;
    surface->ai.ai_placement_enabled = true;
    
    global_compositor.surfaces[global_compositor.num_surfaces++] = surface;
    
    // Add to current workspace
    int ws = global_compositor.current_workspace;
    global_compositor.workspaces[ws].surfaces[global_compositor.workspaces[ws].num_surfaces++] = surface;
    
    spinlock_release(&global_compositor.lock);
    
    kprintf("[Compositor] Created surface #%d at (%d, %d) size %dx%d\n",
            surface->id, surface->x, surface->y, surface->width, surface->height);
    
    return surface;
}

// AI: Smart window placement
void compositor_ai_smart_placement(wayland_surface_t* surface) {
    wayland_output_t* output = global_compositor.outputs[0];
    if (!output) return;
    
    // Strategy 1: Find largest empty area
    // Strategy 2: Avoid overlapping with important windows
    // Strategy 3: Consider screen edges and panels
    
    int best_x = 0;
    int best_y = 0;
    int best_score = 0;
    
    // Grid-based placement evaluation
    int grid_size = 50;
    
    for (int y = 0; y < output->height - surface->height; y += grid_size) {
        for (int x = 0; x < output->width - surface->width; x += grid_size) {
            int score = 100;
            
            // Check overlap with existing windows
            for (int i = 0; i < global_compositor.num_surfaces; i++) {
                wayland_surface_t* other = global_compositor.surfaces[i];
                if (!other->mapped) continue;
                
                // Check if rectangles overlap
                if (x < other->x + other->width &&
                    x + surface->width > other->x &&
                    y < other->y + other->height &&
                    y + surface->height > other->y) {
                    
                    // Penalize overlap
                    score -= 50;
                    
                    // Extra penalty for overlapping focused window
                    if (other->focused) {
                        score -= 30;
                    }
                }
            }
            
            // Prefer center of screen
            int center_x = output->width / 2;
            int center_y = output->height / 2;
            int dist = abs(x - center_x) + abs(y - center_y);
            score -= dist / 100;
            
            // Prefer top-left quadrant slightly
            if (x < output->width / 2 && y < output->height / 2) {
                score += 10;
            }
            
            if (score > best_score) {
                best_score = score;
                best_x = x;
                best_y = y;
            }
        }
    }
    
    surface->x = best_x;
    surface->y = best_y;
    
    kprintf("[Compositor AI] Smart placement: (%d, %d) score=%d\n", 
            best_x, best_y, best_score);
}

// AI: Predict optimal window size
void compositor_ai_predict_window_size(wayland_surface_t* surface, int* width, int* height) {
    wayland_output_t* output = global_compositor.outputs[0];
    if (!output) {
        *width = 800;
        *height = 600;
        return;
    }
    
    // Analyze app_id to determine window type
    if (strstr(surface->app_id, "terminal")) {
        *width = 1000;
        *height = 700;
    } else if (strstr(surface->app_id, "browser")) {
        *width = output->width * 0.8;
        *height = output->height * 0.9;
    } else if (strstr(surface->app_id, "editor") || strstr(surface->app_id, "ide")) {
        *width = output->width * 0.75;
        *height = output->height * 0.85;
    } else if (strstr(surface->app_id, "media") || strstr(surface->app_id, "video")) {
        *width = 1280;
        *height = 720;
    } else {
        // Default size
        *width = 800;
        *height = 600;
    }
    
    // Ensure window fits on screen
    if (*width > output->width) *width = output->width;
    if (*height > output->height) *height = output->height;
}

// Render a frame
void compositor_render_frame(void) {
    wayland_output_t* output = global_compositor.outputs[0];
    if (!output || !output->framebuffer) return;
    
    uint64_t frame_start = rdtsc();
    
    // Clear screen (draw wallpaper)
    uint32_t* fb = (uint32_t*)output->framebuffer;
    
    // Gradient background (AI-selected calming colors)
    for (int y = 0; y < output->height; y++) {
        uint8_t r = 20 + (y * 40 / output->height);
        uint8_t g = 25 + (y * 50 / output->height);
        uint8_t b = 35 + (y * 70 / output->height);
        
        uint32_t color = (r << 16) | (g << 8) | b;
        
        for (int x = 0; x < output->width; x++) {
            fb[y * (output->fb_stride / 4) + x] = color;
        }
    }
    
    // Render all surfaces in current workspace
    int ws = global_compositor.current_workspace;
    for (int i = 0; i < global_compositor.workspaces[ws].num_surfaces; i++) {
        wayland_surface_t* surface = global_compositor.workspaces[ws].surfaces[i];
        if (surface->mapped) {
            compositor_render_surface(surface);
        }
    }
    
    // Draw cursor
    if (global_compositor.cursor_visible) {
        compositor_draw_cursor();
    }
    
    // Update FPS
    uint64_t frame_time = rdtsc() - frame_start;
    global_compositor.frame_count++;
    global_compositor.last_frame_time = frame_time;
    
    uint32_t frame_time_us = frame_time / (cpu_frequency_hz() / 1000000);
    global_compositor.fps = 1000000.0f / frame_time_us;
}

// Render a surface
void compositor_render_surface(wayland_surface_t* surface) {
    if (!surface->buffer) return;
    
    wayland_output_t* output = global_compositor.outputs[0];
    uint32_t* fb = (uint32_t*)output->framebuffer;
    uint32_t* src = (uint32_t*)surface->buffer;
    
    // Draw window decoration (title bar)
    uint32_t title_color = surface->focused ? 0x4A90E2 : 0x666666;
    int title_height = 30;
    
    for (int y = 0; y < title_height; y++) {
        int screen_y = surface->y + y - title_height;
        if (screen_y < 0 || screen_y >= output->height) continue;
        
        for (int x = 0; x < surface->width; x++) {
            int screen_x = surface->x + x;
            if (screen_x < 0 || screen_x >= output->width) continue;
            
            fb[screen_y * (output->fb_stride / 4) + screen_x] = title_color;
        }
    }
    
    // Draw title text (simplified)
    // In real implementation, use font rendering
    
    // Draw window border
    uint32_t border_color = surface->focused ? 0x4A90E2 : 0x444444;
    int border_width = 2;
    
    // Top/bottom borders
    for (int x = 0; x < surface->width; x++) {
        int screen_x = surface->x + x;
        if (screen_x < 0 || screen_x >= output->width) continue;
        
        for (int b = 0; b < border_width; b++) {
            int screen_y_top = surface->y + b;
            int screen_y_bottom = surface->y + surface->height - 1 - b;
            
            if (screen_y_top >= 0 && screen_y_top < output->height) {
                fb[screen_y_top * (output->fb_stride / 4) + screen_x] = border_color;
            }
            
            if (screen_y_bottom >= 0 && screen_y_bottom < output->height) {
                fb[screen_y_bottom * (output->fb_stride / 4) + screen_x] = border_color;
            }
        }
    }
    
    // Left/right borders
    for (int y = 0; y < surface->height; y++) {
        int screen_y = surface->y + y;
        if (screen_y < 0 || screen_y >= output->height) continue;
        
        for (int b = 0; b < border_width; b++) {
            int screen_x_left = surface->x + b;
            int screen_x_right = surface->x + surface->width - 1 - b;
            
            if (screen_x_left >= 0 && screen_x_left < output->width) {
                fb[screen_y * (output->fb_stride / 4) + screen_x_left] = border_color;
            }
            
            if (screen_x_right >= 0 && screen_x_right < output->width) {
                fb[screen_y * (output->fb_stride / 4) + screen_x_right] = border_color;
            }
        }
    }
    
    // Blit surface content
    for (int y = 0; y < surface->buffer_height; y++) {
        int screen_y = surface->y + y;
        if (screen_y < 0 || screen_y >= output->height) continue;
        
        for (int x = 0; x < surface->buffer_width; x++) {
            int screen_x = surface->x + x;
            if (screen_x < 0 || screen_x >= output->width) continue;
            
            uint32_t pixel = src[y * (surface->buffer_stride / 4) + x];
            fb[screen_y * (output->fb_stride / 4) + screen_x] = pixel;
        }
    }
}

// Draw cursor
void compositor_draw_cursor(void) {
    wayland_output_t* output = global_compositor.outputs[0];
    uint32_t* fb = (uint32_t*)output->framebuffer;
    
    int x = global_compositor.cursor_x;
    int y = global_compositor.cursor_y;
    
    // Simple arrow cursor (16x16)
    uint32_t cursor_color = 0xFFFFFF;
    
    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 16 - cy; cx++) {
            int px = x + cx;
            int py = y + cy;
            
            if (px >= 0 && px < output->width && py >= 0 && py < output->height) {
                fb[py * (output->fb_stride / 4) + px] = cursor_color;
            }
        }
    }
}

// Handle pointer motion
void compositor_handle_pointer_motion(int x, int y) {
    global_compositor.cursor_x = x;
    global_compositor.cursor_y = y;
    
    // AI: Predict next cursor position for optimization
    // Could pre-render areas the cursor is likely to move to
    
    // Update surface under cursor
    wayland_surface_t* surface = compositor_get_surface_at(x, y);
    
    // Send events to surface (not implemented in this demo)
}

// Get surface at coordinates
wayland_surface_t* compositor_get_surface_at(int x, int y) {
    int ws = global_compositor.current_workspace;
    
    // Check from top to bottom (reverse order)
    for (int i = global_compositor.workspaces[ws].num_surfaces - 1; i >= 0; i--) {
        wayland_surface_t* surface = global_compositor.workspaces[ws].surfaces[i];
        
        if (!surface->mapped) continue;
        
        if (x >= surface->x && x < surface->x + surface->width &&
            y >= surface->y && y < surface->y + surface->height) {
            return surface;
        }
    }
    
    return NULL;
}

// Focus surface
void compositor_focus_surface(wayland_surface_t* surface) {
    if (!surface) return;
    
    // Unfocus current
    if (global_compositor.focused_surface) {
        global_compositor.focused_surface->focused = false;
    }
    
    // Focus new
    surface->focused = true;
    global_compositor.focused_surface = surface;
    
    // AI: Track window usage
    for (int i = 0; i < 256; i++) {
        if (global_compositor.ai.usage_stats[i].surface_id == surface->id) {
            global_compositor.ai.usage_stats[i].last_used = time(NULL);
            break;
        }
    }
    
    kprintf("[Compositor] Focused surface #%d (%s)\n", surface->id, surface->title);
}

// Main compositor loop
void compositor_run(void) {
    kprintf("[Compositor] Starting main loop\n");
    
    while (global_compositor.running) {
        // Render frame
        compositor_render_frame();
        
        // VSync
        if (global_compositor.vsync_enabled) {
            // Wait for vertical blank
            // (Simplified - would use actual VSync interrupt)
            uint64_t frame_time_us = 1000000 / 60; // 60 FPS
            uint64_t actual_time_us = global_compositor.last_frame_time / (cpu_frequency_hz() / 1000000);
            
            if (actual_time_us < frame_time_us) {
                usleep(frame_time_us - actual_time_us);
            }
        }
        
        // AI: Optimize rendering based on load
        if (global_compositor.ai.performance_optimization) {
            compositor_ai_optimize_rendering();
        }
        
        // Process events (would be from input devices)
        // schedule();
    }
    
    kprintf("[Compositor] Main loop exited\n");
}

// AI: Optimize rendering
void compositor_ai_optimize_rendering(void) {
    // If FPS drops below 50, take action
    if (global_compositor.fps < 50.0f) {
        // Disable expensive effects
        // Reduce render quality
        // Skip rendering of occluded windows
        
        kprintf("[Compositor AI] Low FPS (%.1f), optimizing...\n", global_compositor.fps);
    }
}