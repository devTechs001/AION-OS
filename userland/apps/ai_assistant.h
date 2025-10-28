/**
 * AION OS - AI-Powered Personal Assistant
 * Multi-purpose AI application
 */

 #ifndef AI_ASSISTANT_H
 #define AI_ASSISTANT_H
 
 #include "../../kernel/ai/nlp/bert_engine.h"
 #include "../../kernel/ai/vision/cv_engine.h"
 #include <stdint.h>
 
 // Assistant capabilities
 typedef enum {
     CAPABILITY_NLP,
     CAPABILITY_VISION,
     CAPABILITY_CODE,
     CAPABILITY_FILE_MANAGEMENT,
     CAPABILITY_SYSTEM_CONTROL,
     CAPABILITY_WEB_SEARCH,
     CAPABILITY_AUTOMATION,
     CAPABILITY_LEARNING
 } assistant_capability_t;
 
 // Assistant context
 typedef struct {
     bert_engine_t nlp_engine;
     cv_engine_t vision_engine;
     
     // User preferences
     char user_name[64];
     char preferred_language[16];
     bool learn_from_usage;
     
     // Conversation history
     char** conversation_history;
     uint32_t history_size;
     
     // Automation scripts
     void* automation_engine;
     
     // Knowledge base
     void* knowledge_db;
     
 } ai_assistant_t;
 
 // Command result
 typedef struct {
     bool success;
     char response[1024];
     void* data;
     size_t data_size;
 } command_result_t;
 
 // Initialize assistant
 int ai_assistant_init(ai_assistant_t* assistant, const char* user_name);
 
 // Process natural language command
 command_result_t ai_assistant_process_command(ai_assistant_t* assistant, const char* command);
 
 // Specific capabilities
 command_result_t ai_assistant_manage_files(ai_assistant_t* assistant, const char* request);
 command_result_t ai_assistant_control_system(ai_assistant_t* assistant, const char* request);
 command_result_t ai_assistant_analyze_image(ai_assistant_t* assistant, const uint8_t* image, uint32_t width, uint32_t height);
 command_result_t ai_assistant_code_help(ai_assistant_t* assistant, const char* request);
 
 // Automation
 int ai_assistant_create_automation(ai_assistant_t* assistant, const char* trigger, const char* action);
 int ai_assistant_run_automation(ai_assistant_t* assistant, const char* name);
 
 // Learning
 int ai_assistant_learn_preference(ai_assistant_t* assistant, const char* context, const char* preference);
 
 // Cleanup
 void ai_assistant_cleanup(ai_assistant_t* assistant);
 
 #endif // AI_ASSISTANT_H