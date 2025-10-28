/**
 * AION OS - Embedded AI Model Repository
 * Self-contained model management
 */

 #ifndef MODEL_REPOSITORY_H
 #define MODEL_REPOSITORY_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 // Model metadata
 typedef struct {
     char name[64];
     char version[16];
     char description[256];
     char url[512];  // Download URL
     char hash[65];  // SHA-256 hash for verification
     
     uint64_t size_bytes;
     
     enum {
         MODEL_FORMAT_TFLITE,
         MODEL_FORMAT_ONNX,
         MODEL_FORMAT_PYTORCH,
         MODEL_FORMAT_NATIVE
     } format;
     
     enum {
         MODEL_TYPE_NLP,
         MODEL_TYPE_VISION,
         MODEL_TYPE_CODE,
         MODEL_TYPE_AUDIO,
         MODEL_TYPE_GENERAL
     } type;
     
     bool is_downloaded;
     bool is_cached;
     char local_path[256];
 } model_info_t;
 
 // Model repository
 typedef struct {
     model_info_t* models;
     uint32_t num_models;
     
     char cache_dir[256];
     uint64_t cache_size_bytes;
     uint64_t cache_limit_bytes;
     
     bool auto_download;
     bool verify_hash;
 } model_repo_t;
 
 // Initialize repository
 int model_repo_init(model_repo_t* repo, const char* cache_dir);
 
 // Register built-in models
 int model_repo_register_builtin(model_repo_t* repo);
 
 // List available models
 model_info_t* model_repo_list(model_repo_t* repo, uint32_t* count);
 
 // Get model by name
 model_info_t* model_repo_get(model_repo_t* repo, const char* name);
 
 // Download model
 int model_repo_download(model_repo_t* repo, const char* model_name);
 
 // Load model into memory
 uint8_t* model_repo_load(model_repo_t* repo, const char* model_name, size_t* size);
 
 // Check if model exists locally
 bool model_repo_exists(model_repo_t* repo, const char* model_name);
 
 // Auto-fetch model on demand
 uint8_t* model_repo_get_or_download(model_repo_t* repo, const char* model_name, size_t* size);
 
 // Cache management
 int model_repo_clear_cache(model_repo_t* repo);
 int model_repo_remove_model(model_repo_t* repo, const char* model_name);
 
 // Cleanup
 void model_repo_cleanup(model_repo_t* repo);
 
 #endif // MODEL_REPOSITORY_H