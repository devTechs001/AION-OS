#ifndef AI_COMPOSITOR_H
#define AI_COMPOSITOR_H

#include <stdint.h>
#include <stdbool.h>

// Wayland Surface
typedef struct wayland_surface {
    int id;
    
    // Geometry
    int x, y;
    int width, height;
    
    // Buffer
    void* buffer;
    size_t buffer_size;
    int buffer_width;
    int buffer_height;
    int buffer_stride;
    
    // State
    bool mapped;
    bool focused;
    bool fullscreen;
    bool maximized;
    
    // Window properties
    char title[128];
    char app_id[64];
    
    // Parent (for popup windows)
    struct wayland_surface* parent;
    
    // Children
    struct wayland_surface* children[16];
    int num_children;
    
    // AI enhancements
    struct {
        bool ai_resize_enabled;
        bool ai_placement_enabled;
        int predicted_next_x;
        int predicted_next_y;
    } ai;
} wayland_surface_t;

// Output (Display)
typedef struct {
    int id;
    char name[64];
    
    int width;
    int height;
    int refresh_rate;
    
    // Framebuffer
    void* framebuffer;
    size_t fb_size;
    int fb_stride;
    
    // Connected
    bool connected;
    bool enabled;
} wayland_output_t;

// Input Device
typedef struct {
    int id;
    char name[64];
    
    enum {
        INPUT_KEYBOARD,
        INPUT_MOUSE,
        INPUT_TOUCHPAD,
        INPUT_TOUCHSCREEN
    } type;
    
    // State
    union {
        struct {
            uint32_t modifiers;
            uint32_t pressed_keys[32];
        } keyboard;
        
        struct {
            int x, y;
            uint32_t buttons;
        } pointer;
        
        struct {
            int x, y;
            bool touching;
        } touch;
    } state;
} wayland_input_t;

// Compositor
typedef struct {
    // Outputs
    wayland_output_t* outputs[8];
    int num_outputs;
    
    // Surfaces
    wayland_surface_t* surfaces[256];
    int num_surfaces;
    wayland_surface_t* focused_surface;
    
    // Input
    wayland_input_t* input_devices[32];
    int num_input_devices;
    
    // Cursor
    int cursor_x;
    int cursor_y;
    bool cursor_visible;
    
    // Workspaces
    struct {
        wayland_surface_t* surfaces[64];
        int num_surfaces;
        bool active;
    } workspaces[10];
    int current_workspace;
    
    // AI features
    struct {
        bool smart_window_placement;
        bool auto_tiling;
        bool gesture_prediction;
        bool performance_optimization;
        
        // Window usage tracking
        struct {
            int surface_id;
            uint64_t focus_time_ms;
            uint64_t last_used;
        } usage_stats[256];
    } ai;
    
    // Rendering
    bool vsync_enabled;
    uint32_t frame_count;
    uint64_t last_frame_time;
    float fps;
    
    bool running;
    spinlock_t lock;
} wayland_compositor_t;

// Function Prototypes
void compositor_init(void);
void compositor_run(void);
void compositor_shutdown(void);

// Surface Management
wayland_surface_t* compositor_create_surface(void);
void compositor_destroy_surface(wayland_surface_t* surface);
void compositor_commit_surface(wayland_surface_t* surface);
void compositor_attach_buffer(wayland_surface_t* surface, void* buffer, 
                              int width, int height, int stride);

// Window Management
void compositor_set_title(wayland_surface_t* surface, const char* title);
void compositor_set_fullscreen(wayland_surface_t* surface, bool fullscreen);
void compositor_set_maximized(wayland_surface_t* surface, bool maximized);
void compositor_move_surface(wayland_surface_t* surface, int x, int y);
void compositor_resize_surface(wayland_surface_t* surface, int width, int height);

// Focus Management
void compositor_focus_surface(wayland_surface_t* surface);
wayland_surface_t* compositor_get_surface_at(int x, int y);

// Input Handling
void compositor_handle_pointer_motion(int x, int y);
void compositor_handle_pointer_button(uint32_t button, bool pressed);
void compositor_handle_keyboard_key(uint32_t key, bool pressed);

// Workspace Management
void compositor_switch_workspace(int workspace_id);
void compositor_move_to_workspace(wayland_surface_t* surface, int workspace_id);

// Rendering
void compositor_render_frame(void);
void compositor_render_surface(wayland_surface_t* surface);
void compositor_draw_cursor(void);

// AI Features
void compositor_ai_smart_placement(wayland_surface_t* surface);
void compositor_ai_auto_tile(void);
void compositor_ai_predict_window_size(wayland_surface_t* surface, int* width, int* height);
void compositor_ai_optimize_rendering(void);

// Effects
void compositor_animate_window(wayland_surface_t* surface, int target_x, int target_y);
void compositor_fade_in(wayland_surface_t* surface);
void compositor_fade_out(wayland_surface_t* surface);

#endif // AI_COMPOSITOR_H