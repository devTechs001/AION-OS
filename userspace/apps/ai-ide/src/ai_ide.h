#ifndef AI_IDE_H
#define AI_IDE_H

#include <stdint.h>
#include <stdbool.h>

// Code Language
typedef enum {
    LANG_C,
    LANG_CPP,
    LANG_PYTHON,
    LANG_RUST,
    LANG_JAVASCRIPT,
    LANG_GO,
    LANG_UNKNOWN
} code_language_t;

// Syntax Token
typedef struct {
    char text[128];
    int type;  // keyword, identifier, operator, etc.
    int line;
    int column;
    int length;
} syntax_token_t;

// Code Completion Suggestion
typedef struct {
    char completion[256];
    char description[512];
    char type[64];  // function, variable, class, etc.
    float confidence;
    int priority;
} code_completion_t;

// Diagnostic (Error/Warning)
typedef struct {
    int line;
    int column;
    int severity;  // 0=hint, 1=warning, 2=error
    char message[512];
    char fix_suggestion[256];
} code_diagnostic_t;

// Code Editor Buffer
typedef struct {
    char** lines;
    int num_lines;
    int capacity;
    
    char filename[256];
    code_language_t language;
    bool modified;
    
    // Cursor position
    int cursor_line;
    int cursor_column;
    
    // Selection
    int selection_start_line;
    int selection_start_column;
    int selection_end_line;
    int selection_end_column;
    bool has_selection;
    
    // Syntax highlighting
    syntax_token_t* tokens;
    int num_tokens;
    
    // Diagnostics
    code_diagnostic_t* diagnostics;
    int num_diagnostics;
} code_buffer_t;

// AI Code Intelligence
typedef struct {
    // Code completion model
    void* completion_model;  // Neural network
    
    // Semantic analysis
    void* ast;  // Abstract Syntax Tree
    
    // Code context
    char** recent_files;
    int num_recent_files;
    
    // Statistics
    uint64_t completions_provided;
    uint64_t completions_accepted;
    float acceptance_rate;
    
    // Cache
    code_completion_t* completion_cache[1024];
    int cache_size;
} ai_code_intelligence_t;

// AI Debugger
typedef struct {
    int current_breakpoint_line;
    
    // Variable watch
    struct {
        char name[64];
        char value[256];
        char type[64];
    } watched_variables[32];
    int num_watched;
    
    // Call stack
    struct {
        char function[128];
        char file[256];
        int line;
    } call_stack[64];
    int stack_depth;
    
    // AI predictions
    struct {
        int predicted_crash_line;
        float crash_probability;
        char* fix_suggestions[8];
        int num_suggestions;
    } ai_analysis;
} ai_debugger_t;

// AI Code Generator
typedef struct {
    char* prompt;
    char* generated_code;
    size_t code_length;
    
    code_language_t target_language;
    
    // Generation parameters
    float temperature;     // Creativity (0.0-1.0)
    int max_tokens;
    int num_alternatives;
    
    // Quality metrics
    float syntactic_correctness;
    float semantic_correctness;
    int estimated_performance;
} ai_code_generator_t;

// IDE State
typedef struct {
    code_buffer_t* buffers[64];
    int num_buffers;
    int active_buffer;
    
    ai_code_intelligence_t intelligence;
    ai_debugger_t debugger;
    
    // UI state
    int window_width;
    int window_height;
    int scroll_offset;
    
    // Settings
    bool auto_complete_enabled;
    bool auto_format_enabled;
    bool ai_suggestions_enabled;
    int tab_size;
    
    bool running;
} ai_ide_t;

// Function Prototypes
void ai_ide_init(void);
void ai_ide_run(void);

// Buffer Management
code_buffer_t* ai_ide_open_file(const char* filename);
void ai_ide_close_buffer(code_buffer_t* buffer);
void ai_ide_save_buffer(code_buffer_t* buffer);

// Editing
void ai_ide_insert_text(code_buffer_t* buffer, const char* text);
void ai_ide_delete_char(code_buffer_t* buffer);
void ai_ide_move_cursor(code_buffer_t* buffer, int line, int column);

// AI Code Completion
code_completion_t** ai_ide_get_completions(code_buffer_t* buffer, int* num_completions);
void ai_ide_apply_completion(code_buffer_t* buffer, code_completion_t* completion);

// AI Code Generation
char* ai_ide_generate_code(const char* prompt, code_language_t language);
char* ai_ide_generate_function(const char* description, const char* signature);
char* ai_ide_generate_tests(const char* function_code);
char* ai_ide_refactor_code(const char* code, const char* refactor_type);

// AI Debugging
void ai_ide_analyze_code(code_buffer_t* buffer);
code_diagnostic_t** ai_ide_find_bugs(code_buffer_t* buffer, int* num_bugs);
char* ai_ide_suggest_fix(code_diagnostic_t* diagnostic);

// Syntax Analysis
void ai_ide_tokenize(code_buffer_t* buffer);
void ai_ide_build_ast(code_buffer_t* buffer);
code_language_t ai_ide_detect_language(const char* filename);

// Utilities
void ai_ide_format_code(code_buffer_t* buffer);
void ai_ide_auto_indent(code_buffer_t* buffer, int line);

#endif // AI_IDE_H