// AION OS AI-Powered Window Manager
#include "window_manager.h"
#include "../graphics/framebuffer.h"
#include "../input/mouse.h"
#include "../input/keyboard.h"
#include "../ai/predictor.h"

// Window manager state
typedef struct {
    window_t *windows[MAX_WINDOWS];
    uint32_t num_windows;
    window_t *focused_window;
    window_t *dragging_window;
    int drag_offset_x, drag_offset_y;
    bool running;
} wm_state_t;

static wm_state_t wm_state;
static ai_wm_optimizer_t *wm_optimizer;

// Desktop environment
typedef struct {
    uint32_t bg_color;
    char wallpaper_path[256];
    uint32_t *wallpaper_data;
    panel_t *top_panel;
    panel_t *bottom_panel;
    dock_t *dock;
} desktop_t;

static desktop_t desktop;

// Initialize window manager
void wm_init(void) {
    kprintf("[WM] Initializing window manager...\n");
    
    // Clear state
    memset(&wm_state, 0, sizeof(wm_state));
    wm_state.running = true;
    
    // Initialize AI optimizer
    wm_optimizer = ai_wm_optimizer_create();
    
    // Setup desktop
    desktop.bg_color = 0x2E3440;  // Nord theme
    desktop.top_panel = create_panel(PANEL_TOP);
    desktop.bottom_panel = NULL;  // Optional
    desktop.dock = create_dock();
    
    // Register input handlers
    mouse_register_handler(wm_mouse_handler);
    keyboard_register_handler(wm_keyboard_handler);
    
    // Create initial desktop window
    create_desktop_window();
    
    kprintf("[WM] Window manager initialized\n");
}

// Main event loop
void wm_run(void) {
    kprintf("[WM] Starting window manager event loop...\n");
    
    while (wm_state.running) {
        // AI prediction: Optimize event processing
        if (wm_optimizer->should_process_events()) {
            process_events();
        }
        
        // Redraw if needed
        if (needs_redraw()) {
            wm_redraw();
        }
        
        // AI power management
        wm_optimizer->optimize_idle();
        
        // Small delay to prevent busy waiting
        sleep_ms(1);
    }
}

// Create window
window_t* wm_create_window(int x, int y, int width, int height,
                           const char *title, uint32_t flags) {
    if (wm_state.num_windows >= MAX_WINDOWS) {
        return NULL;
    }
    
    window_t *win = (window_t*)kmalloc(sizeof(window_t));
    if (!win) return NULL;
    
    // Initialize window
    win->id = wm_state.num_windows + 1;
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->flags = flags;
    strncpy(win->title, title, WINDOW_TITLE_MAX);
    
    // Allocate window buffer
    win->buffer = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
    if (!win->buffer) {
        kfree(win);
        return NULL;
    }
    
    // Clear window buffer
    memset(win->buffer, 0xFF, width * height * sizeof(uint32_t));
    
    // Add decorations if needed
    if (!(flags & WINDOW_FLAG_NO_DECORATIONS)) {
        win->title_bar_height = 30;
        win->border_width = 2;
    } else {
        win->title_bar_height = 0;
        win->border_width = 0;
    }
    
    // Add to window list
    wm_state.windows[wm_state.num_windows++] = win;
    
    // Focus new window
    wm_focus_window(win);
    
    // AI prediction: Predict window layout preferences
    wm_optimizer->predict_window_placement(win);
    
    kprintf("[WM] Created window: %s (%dx%d)\n", title, width, height);
    
    return win;
}

// Destroy window
void wm_destroy_window(window_t *win) {
    if (!win) return;
    
    // Remove from window list
    for (uint32_t i = 0; i < wm_state.num_windows; i++) {
        if (wm_state.windows[i] == win) {
            // Shift remaining windows
            for (uint32_t j = i; j < wm_state.num_windows - 1; j++) {
                wm_state.windows[j] = wm_state.windows[j + 1];
            }
            wm_state.num_windows--;
            break;
        }
    }
    
    // Free window resources
    if (win->buffer) {
        kfree(win->buffer);
    }
    
    kfree(win);
    
    // Focus another window
    if (wm_state.num_windows > 0) {
        wm_focus_window(wm_state.windows[wm_state.num_windows - 1]);
    }
}

// Draw window
void wm_draw_window(window_t *win) {
    if (!win || !(win->flags & WINDOW_FLAG_VISIBLE)) {
        return;
    }
    
    // Draw decorations
    if (!(win->flags & WINDOW_FLAG_NO_DECORATIONS)) {
        draw_window_decorations(win);
    }
    
    // Draw window content
    framebuffer_blit(win->buffer, 0, 0,
                    win->x + win->border_width,
                    win->y + win->title_bar_height + win->border_width,
                    win->width, win->height, win->width * 4);
}

// Draw window decorations
void draw_window_decorations(window_t *win) {
    bool is_focused = (win == wm_state.focused_window);
    
    // Title bar colors
    uint32_t title_color = is_focused ? 0x5E81AC : 0x4C566A;  // Nord theme
    uint32_t text_color = 0xECEFF4;
    uint32_t border_color = is_focused ? 0x88C0D0 : 0x3B4252;
    
    // Draw border
    framebuffer_draw_rect(win->x, win->y,
                         win->width + 2 * win->border_width,
                         win->height + win->title_bar_height + 2 * win->border_width,
                         border_color);
    
    // Draw title bar
    framebuffer_fill_rect(win->x + win->border_width,
                         win->y + win->border_width,
                         win->width, win->title_bar_height,
                         title_color);
    
    // Draw title text
    framebuffer_draw_text(win->title,
                         win->x + win->border_width + 10,
                         win->y + win->border_width + 7,
                         text_color);
    
    // Draw window buttons (close, minimize, maximize)
    draw_window_buttons(win);
}

// Draw window buttons
void draw_window_buttons(window_t *win) {
    int button_size = 16;
    int button_margin = 5;
    int button_y = win->y + win->border_width + 7;
    
    // Close button (red)
    int close_x = win->x + win->width - button_size - button_margin;
    framebuffer_fill_rect(close_x, button_y, button_size, button_size, 0xBF616A);
    framebuffer_draw_text("×", close_x + 4, button_y + 1, 0xFFFFFF);
    
    // Maximize button (green)
    int max_x = close_x - button_size - button_margin;
    framebuffer_fill_rect(max_x, button_y, button_size, button_size, 0xA3BE8C);
    framebuffer_draw_text("□", max_x + 4, button_y + 1, 0xFFFFFF);
    
    // Minimize button (yellow)
    int min_x = max_x - button_size - button_margin;
    framebuffer_fill_rect(min_x, button_y, button_size, button_size, 0xEBCB8B);
    framebuffer_draw_text("−", min_x + 4, button_y + 1, 0xFFFFFF);
}

// Redraw entire screen
void wm_redraw(void) {
    // AI optimization: Dirty rectangle tracking
    dirty_rect_list_t *dirty_rects = wm_optimizer->get_dirty_rects();
    
    if (dirty_rects->full_redraw) {
        // Full redraw
        draw_desktop();
        
        // Draw all windows from back to front
        for (uint32_t i = 0; i < wm_state.num_windows; i++) {
            wm_draw_window(wm_state.windows[i]);
        }
        
        // Draw panels
        if (desktop.top_panel) {
            draw_panel(desktop.top_panel);
        }
        if (desktop.bottom_panel) {
            draw_panel(desktop.bottom_panel);
        }
        
        // Draw dock
        if (desktop.dock) {
            draw_dock(desktop.dock);
        }
        
        // Draw cursor
        draw_cursor();
    } else {
        // Partial redraw (only dirty rectangles)
        for (uint32_t i = 0; i < dirty_rects->count; i++) {
            redraw_rect(&dirty_rects->rects[i]);
        }
    }
    
    // Swap buffers
    framebuffer_swap_buffers();
}

// Mouse handler
void wm_mouse_handler(mouse_event_t *event) {
    // Update cursor position
    int mouse_x = event->x;
    int mouse_y = event->y;
    
    // AI prediction: Predict window that will be clicked
    window_t *predicted_win = wm_optimizer->predict_window_click(mouse_x, mouse_y);
    if (predicted_win) {
        wm_prefetch_window_resources(predicted_win);
    }
    
    if (event->buttons & MOUSE_BUTTON_LEFT) {
        if (!wm_state.dragging_window) {
            // Start dragging
            window_t *win = find_window_at(mouse_x, mouse_y);
            if (win) {
                // Check if clicking title bar
                if (mouse_y >= win->y && 
                    mouse_y < win->y + win->title_bar_height) {
                    wm_state.dragging_window = win;
                    wm_state.drag_offset_x = mouse_x - win->x;
                    wm_state.drag_offset_y = mouse_y - win->y;
                    wm_focus_window(win);
                }
            }
        } else {
            // Continue dragging
            wm_state.dragging_window->x = mouse_x - wm_state.drag_offset_x;
            wm_state.dragging_window->y = mouse_y - wm_state.drag_offset_y;
        }
    } else {
        // Stop dragging
        wm_state.dragging_window = NULL;
    }
    
    // Handle window button clicks
    if (event->buttons & MOUSE_BUTTON_LEFT) {
        handle_button_click(mouse_x, mouse_y);
    }
}

// Keyboard handler
void wm_keyboard_handler(keyboard_event_t *event) {
    // Send to focused window
    if (wm_state.focused_window && event->type == KEY_PRESS) {
        window_send_event(wm_state.focused_window, EVENT_KEYPRESS, event);
    }
    
    // Handle hotkeys
    if (event->modifiers & KEY_MOD_ALT) {
        switch (event->keycode) {
            case KEY_TAB:
                // Alt+Tab: Switch windows
                wm_switch_window();
                break;
            case KEY_F4:
                // Alt+F4: Close window
                if (wm_state.focused_window) {
                    wm_destroy_window(wm_state.focused_window);
                }
                break;
        }
    }
}

// Focus window
void wm_focus_window(window_t *win) {
    if (wm_state.focused_window == win) return;
    
    // Unfocus old window
    if (wm_state.focused_window) {
        wm_state.focused_window->flags &= ~WINDOW_FLAG_FOCUSED;
        window_send_event(wm_state.focused_window, EVENT_UNFOCUS, NULL);
    }
    
    // Focus new window
    wm_state.focused_window = win;
    if (win) {
        win->flags |= WINDOW_FLAG_FOCUSED;
        window_send_event(win, EVENT_FOCUS, NULL);
        
        // Bring to front
        bring_to_front(win);
    }
}

// Create panel
panel_t* create_panel(panel_type_t type) {
    panel_t *panel = (panel_t*)kmalloc(sizeof(panel_t));
    if (!panel) return NULL;
    
    panel->type = type;
    panel->height = 32;
    panel->y = (type == PANEL_TOP) ? 0 : (fb_info.height - panel->height);
    panel->width = fb_info.width;
    panel->bg_color = 0x2E3440;  // Nord theme
    
    // Create panel widgets
    panel->num_widgets = 0;
    
    // Add clock widget
    panel_add_widget(panel, create_clock_widget());
    
    // Add app launcher
    panel_add_widget(panel, create_launcher_widget());
    
    // Add system tray
    panel_add_widget(panel, create_systray_widget());
    
    return panel;
}

// Draw panel
void draw_panel(panel_t *panel) {
    // Draw panel background
    framebuffer_fill_rect(0, panel->y, panel->width, panel->height,
                         panel->bg_color);
    
    // Draw widgets
    int x = 10;
    for (uint32_t i = 0; i < panel->num_widgets; i++) {
        widget_t *widget = panel->widgets[i];
        widget->draw(widget, x, panel->y + 4);
        x += widget->width + 10;
    }
}