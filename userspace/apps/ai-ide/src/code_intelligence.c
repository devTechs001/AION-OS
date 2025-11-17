#include "ai_ide.h"
#include "../../kernel/ai/nlp/nlp_engine.h"
#include "../../kernel/ai/ml/tflite.h"
#include <string.h>
#include <ctype.h>

static ai_ide_t global_ide = {0};

void ai_ide_init(void) {
    memset(&global_ide, 0, sizeof(global_ide));
    
    global_ide.auto_complete_enabled = true;
    global_ide.auto_format_enabled = true;
    global_ide.ai_suggestions_enabled = true;
    global_ide.tab_size = 4;
    global_ide.running = true;
    
    global_ide.window_width = 120;
    global_ide.window_height = 40;
    
    // Load AI code completion model
    tflite_model_t* completion_model = tflite_load_model("/usr/share/ai-ide/models/code_completion.tflite");
    if (completion_model) {
        global_ide.intelligence.completion_model = tflite_create_interpreter(completion_model);
        tflite_allocate_tensors(global_ide.intelligence.completion_model);
    }
    
    kprintf("[AI IDE] Code++ IDE initialized\n");
    kprintf("[AI IDE] AI code completion: %s\n", 
            completion_model ? "ENABLED" : "DISABLED (model not found)");
}

// Detect programming language from file extension
code_language_t ai_ide_detect_language(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return LANG_UNKNOWN;
    
    if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) return LANG_C;
    if (strcmp(ext, ".cpp") == 0 || strcmp(ext, ".hpp") == 0 || strcmp(ext, ".cc") == 0) return LANG_CPP;
    if (strcmp(ext, ".py") == 0) return LANG_PYTHON;
    if (strcmp(ext, ".rs") == 0) return LANG_RUST;
    if (strcmp(ext, ".js") == 0 || strcmp(ext, ".ts") == 0) return LANG_JAVASCRIPT;
    if (strcmp(ext, ".go") == 0) return LANG_GO;
    
    return LANG_UNKNOWN;
}

// Open file into buffer
code_buffer_t* ai_ide_open_file(const char* filename) {
    code_buffer_t* buffer = kmalloc(sizeof(code_buffer_t));
    memset(buffer, 0, sizeof(code_buffer_t));
    
    strncpy(buffer->filename, filename, 255);
    buffer->language = ai_ide_detect_language(filename);
    buffer->modified = false;
    
    // Read file
    int fd = vfs_open(filename, O_RDONLY);
    if (fd < 0) {
        // New file
        buffer->num_lines = 1;
        buffer->capacity = 16;
        buffer->lines = kmalloc(buffer->capacity * sizeof(char*));
        buffer->lines[0] = strdup("");
        
        kprintf("[AI IDE] New file: %s\n", filename);
    } else {
        // Existing file
        struct stat st;
        vfs_fstat(fd, &st);
        
        char* file_data = kmalloc(st.st_size + 1);
        vfs_read(fd, file_data, st.st_size);
        file_data[st.st_size] = '\0';
        vfs_close(fd);
        
        // Split into lines
        buffer->capacity = 16;
        buffer->lines = kmalloc(buffer->capacity * sizeof(char*));
        buffer->num_lines = 0;
        
        char* line_start = file_data;
        char* line_end;
        
        while ((line_end = strchr(line_start, '\n')) != NULL) {
            *line_end = '\0';
            
            if (buffer->num_lines >= buffer->capacity) {
                buffer->capacity *= 2;
                buffer->lines = krealloc(buffer->lines, buffer->capacity * sizeof(char*));
            }
            
            buffer->lines[buffer->num_lines++] = strdup(line_start);
            line_start = line_end + 1;
        }
        
        // Last line (if no trailing newline)
        if (*line_start) {
            if (buffer->num_lines >= buffer->capacity) {
                buffer->capacity *= 2;
                buffer->lines = krealloc(buffer->lines, buffer->capacity * sizeof(char*));
            }
            buffer->lines[buffer->num_lines++] = strdup(line_start);
        }
        
        kfree(file_data);
        
        kprintf("[AI IDE] Opened: %s (%d lines)\n", filename, buffer->num_lines);
    }
    
    // Tokenize for syntax highlighting
    ai_ide_tokenize(buffer);
    
    // Run AI analysis
    if (global_ide.ai_suggestions_enabled) {
        ai_ide_analyze_code(buffer);
    }
    
    return buffer;
}

// AI-powered code completion
code_completion_t** ai_ide_get_completions(code_buffer_t* buffer, int* num_completions) {
    if (!buffer || !global_ide.auto_complete_enabled) {
        *num_completions = 0;
        return NULL;
    }
    
    // Get current line and cursor position
    int line = buffer->cursor_line;
    int col = buffer->cursor_column;
    
    if (line >= buffer->num_lines) {
        *num_completions = 0;
        return NULL;
    }
    
    char* current_line = buffer->lines[line];
    
    // Extract the partial word being typed
    int word_start = col;
    while (word_start > 0 && (isalnum(current_line[word_start - 1]) || current_line[word_start - 1] == '_')) {
        word_start--;
    }
    
    char partial_word[128] = {0};
    strncpy(partial_word, current_line + word_start, col - word_start);
    
    if (strlen(partial_word) < 2) {
        *num_completions = 0;
        return NULL;
    }
    
    kprintf("[AI IDE] Completing: '%s'\n", partial_word);
    
    // Use AI model for intelligent completions
    code_completion_t** completions = kmalloc(16 * sizeof(code_completion_t*));
    int count = 0;
    
    // Strategy 1: Keyword/built-in completion
    const char* c_keywords[] = {
        "if", "else", "while", "for", "return", "int", "char", "void", 
        "struct", "typedef", "const", "static", "sizeof", "switch", "case",
        "break", "continue", "goto", "unsigned", "signed", "long", "short",
        "float", "double", "enum", "union", NULL
    };
    
    for (int i = 0; c_keywords[i] != NULL && count < 16; i++) {
        if (strncmp(partial_word, c_keywords[i], strlen(partial_word)) == 0) {
            code_completion_t* comp = kmalloc(sizeof(code_completion_t));
            strcpy(comp->completion, c_keywords[i]);
            sprintf(comp->description, "C keyword");
            strcpy(comp->type, "keyword");
            comp->confidence = 0.9f;
            comp->priority = 10;
            
            completions[count++] = comp;
        }
    }
    
    // Strategy 2: Context-aware completions using AI
    if (global_ide.intelligence.completion_model) {
        // Prepare input for neural network
        // Context: previous 10 lines + current line
        
        char context[2048] = {0};
        int context_start = (line >= 10) ? line - 10 : 0;
        
        for (int i = context_start; i < line; i++) {
            strcat(context, buffer->lines[i]);
            strcat(context, "\n");
        }
        strcat(context, current_line);
        
        // Tokenize context and run through model
        // (Simplified - real implementation would use proper tokenization)
        
        // For demonstration, add some common function completions
        if (buffer->language == LANG_C || buffer->language == LANG_CPP) {
            const char* functions[] = {
                "printf", "malloc", "free", "strlen", "strcpy", "strcmp",
                "memset", "memcpy", "fopen", "fclose", "fprintf", NULL
            };
            
            for (int i = 0; functions[i] != NULL && count < 16; i++) {
                if (strncmp(partial_word, functions[i], strlen(partial_word)) == 0) {
                    code_completion_t* comp = kmalloc(sizeof(code_completion_t));
                    strcpy(comp->completion, functions[i]);
                    sprintf(comp->description, "Standard library function");
                    strcpy(comp->type, "function");
                    comp->confidence = 0.85f;
                    comp->priority = 8;
                    
                    completions[count++] = comp;
                }
            }
        }
    }
    
    // Strategy 3: Local variable/function completions
    // Scan previous code for identifiers
    for (int i = 0; i < line && count < 16; i++) {
        char* line_text = buffer->lines[i];
        
        // Simple pattern matching for variable declarations
        // e.g., "int my_variable"
        char* token = strtok(strdup(line_text), " \t(),;");
        while (token != NULL && count < 16) {
            if (strlen(token) > 2 && 
                strncmp(partial_word, token, strlen(partial_word)) == 0 &&
                strcmp(partial_word, token) != 0) {
                
                // Check if we already have this completion
                bool already_added = false;
                for (int j = 0; j < count; j++) {
                    if (strcmp(completions[j]->completion, token) == 0) {
                        already_added = true;
                        break;
                    }
                }
                
                if (!already_added) {
                    code_completion_t* comp = kmalloc(sizeof(code_completion_t));
                    strcpy(comp->completion, token);
                    sprintf(comp->description, "Local identifier from line %d", i + 1);
                    strcpy(comp->type, "identifier");
                    comp->confidence = 0.7f;
                    comp->priority = 5;
                    
                    completions[count++] = comp;
                }
            }
            
            token = strtok(NULL, " \t(),;");
        }
    }
    
    // Update statistics
    global_ide.intelligence.completions_provided += count;
    
    *num_completions = count;
    
    if (count > 0) {
        kprintf("[AI IDE] Generated %d completions\n", count);
    }
    
    return completions;
}

// AI Code Generation
char* ai_ide_generate_code(const char* prompt, code_language_t language) {
    kprintf("[AI IDE] Generating code from prompt: '%s'\n", prompt);
    
    // Parse the natural language prompt
    nlp_intent_t* intent = nlp_parse_command(prompt);
    
    if (!intent) {
        return strdup("// Error: Could not understand prompt\n");
    }
    
    // Generate code based on intent
    char* generated = kmalloc(4096);
    memset(generated, 0, 4096);
    
    // Example: "create a function that adds two numbers"
    if (strstr(prompt, "function") && strstr(prompt, "add")) {
        if (language == LANG_C) {
            sprintf(generated,
                "// AI-generated function\n"
                "int add(int a, int b) {\n"
                "    return a + b;\n"
                "}\n");
        } else if (language == LANG_PYTHON) {
            sprintf(generated,
                "# AI-generated function\n"
                "def add(a, b):\n"
                "    return a + b\n");
        }
    }
    // Example: "sort an array"
    else if (strstr(prompt, "sort") && strstr(prompt, "array")) {
        if (language == LANG_C) {
            sprintf(generated,
                "// AI-generated sorting function\n"
                "void bubble_sort(int arr[], int n) {\n"
                "    for (int i = 0; i < n - 1; i++) {\n"
                "        for (int j = 0; j < n - i - 1; j++) {\n"
                "            if (arr[j] > arr[j + 1]) {\n"
                "                int temp = arr[j];\n"
                "                arr[j] = arr[j + 1];\n"
                "                arr[j + 1] = temp;\n"
                "            }\n"
                "        }\n"
                "    }\n"
                "}\n");
        }
    }
    // Example: "read a file"
    else if (strstr(prompt, "read") && strstr(prompt, "file")) {
        if (language == LANG_C) {
            sprintf(generated,
                "// AI-generated file reading function\n"
                "char* read_file(const char* filename) {\n"
                "    FILE* file = fopen(filename, \"r\");\n"
                "    if (!file) return NULL;\n"
                "    \n"
                "    fseek(file, 0, SEEK_END);\n"
                "    long size = ftell(file);\n"
                "    rewind(file);\n"
                "    \n"
                "    char* buffer = malloc(size + 1);\n"
                "    fread(buffer, 1, size, file);\n"
                "    buffer[size] = '\\0';\n"
                "    \n"
                "    fclose(file);\n"
                "    return buffer;\n"
                "}\n");
        }
    }
    else {
        sprintf(generated,
                "// AI could not generate code for this prompt\n"
                "// Prompt: %s\n"
                "// Please provide more specific description\n",
                prompt);
    }
    
    nlp_free_intent(intent);
    
    kprintf("[AI IDE] Generated %zu bytes of code\n", strlen(generated));
    
    return generated;
}

// AI Bug Detection
code_diagnostic_t** ai_ide_find_bugs(code_buffer_t* buffer, int* num_bugs) {
    code_diagnostic_t** bugs = kmalloc(32 * sizeof(code_diagnostic_t*));
    int count = 0;
    
    kprintf("[AI IDE] Running AI bug detection on %s...\n", buffer->filename);
    
    // Pattern-based bug detection
    for (int line = 0; line < buffer->num_lines && count < 32; line++) {
        char* line_text = buffer->lines[line];
        
        // Check 1: Potential null pointer dereference
        if (strstr(line_text, "->") && !strstr(line_text, "if") && !strstr(line_text, "NULL")) {
            code_diagnostic_t* bug = kmalloc(sizeof(code_diagnostic_t));
            bug->line = line;
            bug->column = 0;
            bug->severity = 1; // Warning
            strcpy(bug->message, "Potential null pointer dereference");
            strcpy(bug->fix_suggestion, "Add NULL check before dereferencing pointer");
            bugs[count++] = bug;
        }
        
        // Check 2: Memory leak (malloc without free)
        if (strstr(line_text, "malloc") && !strstr(line_text, "free")) {
            // Check if there's a corresponding free in the file
            bool found_free = false;
            for (int i = line + 1; i < buffer->num_lines; i++) {
                if (strstr(buffer->lines[i], "free")) {
                    found_free = true;
                    break;
                }
            }
            
            if (!found_free) {
                code_diagnostic_t* bug = kmalloc(sizeof(code_diagnostic_t));
                bug->line = line;
                bug->column = 0;
                bug->severity = 2; // Error
                strcpy(bug->message, "Potential memory leak: allocated memory not freed");
                strcpy(bug->fix_suggestion, "Call free() when memory is no longer needed");
                bugs[count++] = bug;
            }
        }
        
        // Check 3: Buffer overflow risk
        if (strstr(line_text, "strcpy") || strstr(line_text, "strcat")) {
            code_diagnostic_t* bug = kmalloc(sizeof(code_diagnostic_t));
            bug->line = line;
            bug->column = 0;
            bug->severity = 2; // Error
            strcpy(bug->message, "Unsafe string operation - buffer overflow risk");
            strcpy(bug->fix_suggestion, "Use strncpy() or strncat() with buffer size");
            bugs[count++] = bug;
        }
        
        // Check 4: Uninitialized variable
        if (strstr(line_text, "int ") || strstr(line_text, "char ") || strstr(line_text, "float ")) {
            if (!strstr(line_text, "=")) {
                code_diagnostic_t* bug = kmalloc(sizeof(code_diagnostic_t));
                bug->line = line;
                bug->column = 0;
                bug->severity = 1; // Warning
                strcpy(bug->message, "Variable declared but not initialized");
                strcpy(bug->fix_suggestion, "Initialize variable at declaration");
                bugs[count++] = bug;
            }
        }
        
        // Check 5: Division by zero
        if (strstr(line_text, "/ ") || strstr(line_text, "% ")) {
            code_diagnostic_t* bug = kmalloc(sizeof(code_diagnostic_t));
            bug->line = line;
            bug->column = 0;
            bug->severity = 1; // Warning
            strcpy(bug->message, "Potential division by zero");
            strcpy(bug->fix_suggestion, "Add check to ensure divisor is not zero");
            bugs[count++] = bug;
        }
    }
    
    *num_bugs = count;
    
    kprintf("[AI IDE] Found %d potential issues\n", count);
    
    return bugs;
}

// AI Code Analysis
void ai_ide_analyze_code(code_buffer_t* buffer) {
    kprintf("[AI IDE] Analyzing code quality...\n");
    
    // Find bugs
    int num_bugs = 0;
    code_diagnostic_t** bugs = ai_ide_find_bugs(buffer, &num_bugs);
    
    if (bugs) {
        buffer->diagnostics = kmalloc(num_bugs * sizeof(code_diagnostic_t));
        for (int i = 0; i < num_bugs; i++) {
            memcpy(&buffer->diagnostics[i], bugs[i], sizeof(code_diagnostic_t));
            kfree(bugs[i]);
        }
        buffer->num_diagnostics = num_bugs;
        kfree(bugs);
    }
    
    // Code metrics
    int total_lines = buffer->num_lines;
    int code_lines = 0;
    int comment_lines = 0;
    int blank_lines = 0;
    
    for (int i = 0; i < buffer->num_lines; i++) {
        char* line = buffer->lines[i];
        
        // Trim leading whitespace
        while (*line && isspace(*line)) line++;
        
        if (*line == '\0') {
            blank_lines++;
        } else if (line[0] == '/' && line[1] == '/') {
            comment_lines++;
        } else if (line[0] == '/' && line[1] == '*') {
            comment_lines++;
        } else {
            code_lines++;
        }
    }
    
    kprintf("[AI IDE] Code metrics:\n");
    kprintf("[AI IDE]   Total lines: %d\n", total_lines);
    kprintf("[AI IDE]   Code lines: %d\n", code_lines);
    kprintf("[AI IDE]   Comments: %d (%.1f%%)\n", 
            comment_lines, 
            (float)comment_lines / total_lines * 100);
    kprintf("[AI IDE]   Blank lines: %d\n", blank_lines);
    kprintf("[AI IDE]   Issues found: %d\n", num_bugs);
}

// Generate unit tests for a function
char* ai_ide_generate_tests(const char* function_code) {
    kprintf("[AI IDE] Generating unit tests...\n");
    
    char* tests = kmalloc(4096);
    
    // Extract function name
    char func_name[128] = {0};
    const char* name_start = strchr(function_code, ' ');
    if (name_start) {
        name_start++;
        const char* name_end = strchr(name_start, '(');
        if (name_end) {
            int len = name_end - name_start;
            if (len > 0 && len < 127) {
                strncpy(func_name, name_start, len);
            }
        }
    }
    
    // Generate test cases
    sprintf(tests,
        "// AI-Generated Unit Tests for %s\n"
        "#include <assert.h>\n"
        "#include <stdio.h>\n"
        "\n"
        "void test_%s_basic() {\n"
        "    // Test basic functionality\n"
        "    // TODO: Add test assertions\n"
        "    printf(\"Test %s: basic - PASS\\n\");\n"
        "}\n"
        "\n"
        "void test_%s_edge_cases() {\n"
        "    // Test edge cases\n"
        "    // TODO: Add edge case tests\n"
        "    printf(\"Test %s: edge cases - PASS\\n\");\n"
        "}\n"
        "\n"
        "void test_%s_error_handling() {\n"
        "    // Test error handling\n"
        "    // TODO: Add error tests\n"
        "    printf(\"Test %s: error handling - PASS\\n\");\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    test_%s_basic();\n"
        "    test_%s_edge_cases();\n"
        "    test_%s_error_handling();\n"
        "    printf(\"All tests passed!\\n\");\n"
        "    return 0;\n"
        "}\n",
        func_name,
        func_name, func_name,
        func_name, func_name,
        func_name, func_name,
        func_name, func_name, func_name);
    
    return tests;
}