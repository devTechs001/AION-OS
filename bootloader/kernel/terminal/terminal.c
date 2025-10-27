// AION OS Advanced Terminal with AI Features
#include "terminal.h"
#include "../ai/nlp.h"

// Terminal state
terminal_t main_terminal;
static ai_terminal_assistant_t *ai_assistant;

// VGA text buffer
static uint16_t *vga_buffer = (uint16_t*)0xB8000;

// Initialize terminal
void terminal_init(void) {
    kprintf("[TERMINAL] Initializing terminal...\n");
    
    // Clear terminal
    main_terminal.row = 0;
    main_terminal.column = 0;
    main_terminal.color = make_color(TERMINAL_COLOR_WHITE, TERMINAL_COLOR_BLACK);
    
    // Initialize AI assistant
    ai_assistant = ai_terminal_assistant_create();
    
    // Clear screen
    terminal_clear();
    
    // Initialize input buffer
    memset(main_terminal.input_buffer, 0, sizeof(main_terminal.input_buffer));
    main_terminal.input_pos = 0;
    
    kprintf("[TERMINAL] Terminal initialized\n");
}

// Print AION OS banner
void terminal_print_banner(void) {
    terminal_set_color(make_color(TERMINAL_COLOR_CYAN, TERMINAL_COLOR_BLACK));
    terminal_print("================================================================================\n");
    terminal_print("     _    ___ ___  _   _    ___  ____     __     __  ___   ___  \n");
    terminal_print("    / \\  |_ _/ _ \\| \\ | |  / _ \\/ ___|    \\ \\   / / |_ _| / _ \\ \n");
    terminal_print("   / _ \\  | | | | |  \\| | | | | \\___ \\     \\ \\ / /   | | | | | |\n");
    terminal_print("  / ___ \\ | | |_| | |\\  | | |_| |___) |     \\ V /    | | | |_| |\n");
    terminal_print(" /_/   \\_\\___\\___/|_| \\_|  \\___/|____/       \\_/    |___(_)___/ \n");
    terminal_print("\n");
    terminal_set_color(make_color(TERMINAL_COLOR_GREEN, TERMINAL_COLOR_BLACK));
    terminal_print(" AI-Powered Operating System v1.0.0 - Build ");
    terminal_print(__DATE__);
    terminal_print("\n");
    terminal_set_color(make_color(TERMINAL_COLOR_WHITE, TERMINAL_COLOR_BLACK));
    terminal_print("================================================================================\n\n");
    
    terminal_print("Welcome to AION OS! Type 'help' for commands or use natural language.\n");
    terminal_print("AI Assistant is ready. Try: \"show me system information\"\n\n");
}

// Process input with AI
void terminal_process_input(char c) {
    if (c == '\n') {
        // Process command
        terminal_print("\n");
        
        // Check if it's a natural language query
        if (is_natural_language(main_terminal.input_buffer)) {
            handle_ai_command(main_terminal.input_buffer);
        } else {
            // Traditional command processing
            execute_command(main_terminal.input_buffer);
        }
        
        // Clear input buffer
        memset(main_terminal.input_buffer, 0, sizeof(main_terminal.input_buffer));
        main_terminal.input_pos = 0;
        
        // Print prompt
        terminal_print_prompt();
    } else if (c == '\b') {
        // Backspace
        if (main_terminal.input_pos > 0) {
            main_terminal.input_pos--;
            main_terminal.input_buffer[main_terminal.input_pos] = '\0';
            terminal_backspace();
        }
    } else {
        // Regular character
        if (main_terminal.input_pos < INPUT_BUFFER_SIZE - 1) {
            main_terminal.input_buffer[main_terminal.input_pos++] = c;
            terminal_putchar(c);
        }
    }
}

// Handle AI commands
void handle_ai_command(const char *input) {
    terminal_set_color(make_color(TERMINAL_COLOR_MAGENTA, TERMINAL_COLOR_BLACK));
    terminal_print("[AI] Processing: ");
    terminal_print(input);
    terminal_print("\n");
    
    // Get AI response
    ai_response_t response = ai_assistant->process_query(input);
    
    // Execute AI-determined action
    switch (response.action) {
        case AI_ACTION_SYSTEM_INFO:
            show_system_info();
            break;
            
        case AI_ACTION_PROCESS_LIST:
            show_process_list();
            break;
            
        case AI_ACTION_MEMORY_STATUS:
            show_memory_status();
            break;
            
        case AI_ACTION_HELP:
            show_help();
            break;
            
        case AI_ACTION_EXECUTE:
            execute_command(response.command);
            break;
            
        case AI_ACTION_EXPLAIN:
            terminal_set_color(make_color(TERMINAL_COLOR_YELLOW, TERMINAL_COLOR_BLACK));
            terminal_print("[AI] ");
            terminal_print(response.explanation);
            terminal_print("\n");
            break;
            
        default:
            terminal_print("[AI] I understand you want to: ");
            terminal_print(response.interpretation);
            terminal_print("\n");
            if (response.suggestions[0]) {
                terminal_print("[AI] Try these commands:\n");
                for (int i = 0; i < 3 && response.suggestions[i]; i++) {
                    terminal_print("  - ");
                    terminal_print(response.suggestions[i]);
                    terminal_print("\n");
                }
            }
    }
    
    terminal_set_color(make_color(TERMINAL_COLOR_WHITE, TERMINAL_COLOR_BLACK));
}

// Print prompt
void terminal_print_prompt(void) {
    terminal_set_color(make_color(TERMINAL_COLOR_GREEN, TERMINAL_COLOR_BLACK));
    terminal_print("aion");
    terminal_set_color(make_color(TERMINAL_COLOR_WHITE, TERMINAL_COLOR_BLACK));
    terminal_print("@");
    terminal_set_color(make_color(TERMINAL_COLOR_CYAN, TERMINAL_COLOR_BLACK));
    terminal_print("localhost");
    terminal_set_color(make_color(TERMINAL_COLOR_WHITE, TERMINAL_COLOR_BLACK));
    terminal_print(":~$ ");
}

// Clear terminal
void terminal_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = make_vgaentry(' ', main_terminal.color);
        }
    }
    main_terminal.row = 0;
    main_terminal.column = 0;
}

// Write character to terminal
void terminal_putchar(char c) {
    if (c == '\n') {
        main_terminal.column = 0;
        main_terminal.row++;
    } else {
        const size_t index = main_terminal.row * VGA_WIDTH + main_terminal.column;
        vga_buffer[index] = make_vgaentry(c, main_terminal.color);
        main_terminal.column++;
        
        if (main_terminal.column >= VGA_WIDTH) {
            main_terminal.column = 0;
            main_terminal.row++;
        }
    }
    
    // Scroll if necessary
    if (main_terminal.row >= VGA_HEIGHT) {
        terminal_scroll();
    }
}

// Scroll terminal
void terminal_scroll(void) {
    // Move all lines up by one
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear last line
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = 
            make_vgaentry(' ', main_terminal.color);
    }
    
    main_terminal.row = VGA_HEIGHT - 1;
}