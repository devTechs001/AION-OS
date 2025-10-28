/**
 * AION OS - Model Repository Implementation
 */

 #include "model_repository.h"
 #include "embedded_models.h"  // Pre-compiled models
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/stat.h>
 
 // Built-in model registry
 static const model_info_t BUILTIN_MODELS[] = {
     {
         .name = "mobilebert-nlu",
         .version = "1.0",
         .description = "MobileBERT for natural language understanding",
         .url = "https://storage.googleapis.com/aion-models/mobilebert-nlu-v1.tflite",
         .hash = "a1b2c3d4...",  // SHA-256
         .size_bytes = 25 * 1024 * 1024,  // 25MB
         .format = MODEL_FORMAT_TFLITE,
         .type = MODEL_TYPE_NLP
     },
     {
         .name = "codegen-350m",
         .version = "1.0",
         .description = "CodeGen 350M for code completion",
         .url = "https://storage.googleapis.com/aion-models/codegen-350m-v1.tflite",
         .hash = "e5f6g7h8...",
         .size_bytes = 350 * 1024 * 1024,  // 350MB
         .format = MODEL_FORMAT_TFLITE,
         .type = MODEL_TYPE_CODE
     },
     {
         .name = "mobilenet-v3",
         .version = "1.0",
         .description = "MobileNetV3 for image classification",
         .url = "https://storage.googleapis.com/aion-models/mobilenet-v3.tflite",
         .hash = "i9j0k1l2...",
         .size_bytes = 5 * 1024 * 1024,  // 5MB
         .format = MODEL_FORMAT_TFLITE,
         .type = MODEL_TYPE_VISION
     },
     {
         .name = "yolov5-nano",
         .version = "1.0",
         .description = "YOLOv5 Nano for object detection",
         .url = "https://storage.googleapis.com/aion-models/yolov5-nano.tflite",
         .hash = "m3n4o5p6...",
         .size_bytes = 7 * 1024 * 1024,  // 7MB
         .format = MODEL_FORMAT_TFLITE,
         .type = MODEL_TYPE_VISION
     },
     {
         .name = "whisper-tiny",
         .version = "1.0",
         .description = "Whisper Tiny for speech recognition",
         .url = "https://storage.googleapis.com/aion-models/whisper-tiny.tflite",
         .hash = "q7r8s9t0...",
         .size_bytes = 39 * 1024 * 1024,  // 39MB
         .format = MODEL_FORMAT_TFLITE,
         .type = MODEL_TYPE_AUDIO
     }
 };
 
 #define NUM_BUILTIN_MODELS (sizeof(BUILTIN_MODELS) / sizeof(BUILTIN_MODELS[0]))
 
 // Initialize repository
 int model_repo_init(model_repo_t* repo, const char* cache_dir) {
     if (!repo) return -1;
     
     memset(repo, 0, sizeof(model_repo_t));
     
     // Set cache directory
     if (cache_dir) {
         strncpy(repo->cache_dir, cache_dir, sizeof(repo->cache_dir) - 1);
     } else {
         strcpy(repo->cache_dir, "/var/aion/models");
     }
     
     // Create cache directory
     mkdir(repo->cache_dir, 0755);
     
     // Settings
     repo->auto_download = true;
     repo->verify_hash = true;
     repo->cache_limit_bytes = 2ULL * 1024 * 1024 * 1024;  // 2GB limit
     
     // Register built-in models
     model_repo_register_builtin(repo);
     
     printf("[ModelRepo] Initialized at %s\n", repo->cache_dir);
     printf("[ModelRepo] %u models available\n", repo->num_models);
     
     return 0;
 }
 
 // Register built-in models
 int model_repo_register_builtin(model_repo_t* repo) {
     if (!repo) return -1;
     
     repo->num_models = NUM_BUILTIN_MODELS;
     repo->models = calloc(NUM_BUILTIN_MODELS, sizeof(model_info_t));
     
     if (!repo->models) return -1;
     
     // Copy built-in model info
     for (uint32_t i = 0; i < NUM_BUILTIN_MODELS; i++) {
         memcpy(&repo->models[i], &BUILTIN_MODELS[i], sizeof(model_info_t));
         
         // Set local path
         snprintf(repo->models[i].local_path, sizeof(repo->models[i].local_path),
                  "%s/%s.tflite", repo->cache_dir, repo->models[i].name);
         
         // Check if already downloaded
         struct stat st;
         if (stat(repo->models[i].local_path, &st) == 0) {
             repo->models[i].is_downloaded = true;
             repo->models[i].is_cached = true;
             repo->cache_size_bytes += st.st_size;
         }
     }
     
     return 0;
 }
 
 // List models
 model_info_t* model_repo_list(model_repo_t* repo, uint32_t* count) {
     if (!repo || !count) return NULL;
     
     *count = repo->num_models;
     return repo->models;
 }
 
 // Get model by name
 model_info_t* model_repo_get(model_repo_t* repo, const char* name) {
     if (!repo || !name) return NULL;
     
     for (uint32_t i = 0; i < repo->num_models; i++) {
         if (strcmp(repo->models[i].name, name) == 0) {
             return &repo->models[i];
         }
     }
     
     return NULL;
 }
 
 // Download model (simplified - would use actual HTTP client)
 int model_repo_download(model_repo_t* repo, const char* model_name) {
     if (!repo || !model_name) return -1;
     
     model_info_t* model = model_repo_get(repo, model_name);
     if (!model) {
         fprintf(stderr, "[ModelRepo] Model not found: %s\n", model_name);
         return -1;
     }
     
     if (model->is_downloaded) {
         printf("[ModelRepo] Model already downloaded: %s\n", model_name);
         return 0;
     }
     
     printf("[ModelRepo] Downloading %s (%lu MB)...\n", 
            model_name, model->size_bytes / (1024 * 1024));
     
     // TODO: Implement actual HTTP download
     // For now, simulate download
     
     // In real implementation:
     // 1. Download from model->url
     // 2. Verify hash
     // 3. Save to model->local_path
     
     // Simulate successful download
     model->is_downloaded = true;
     model->is_cached = true;
     repo->cache_size_bytes += model->size_bytes;
     
     printf("[ModelRepo] Downloaded %s\n", model_name);
     
     return 0;
 }
 
 // Load model into memory
 uint8_t* model_repo_load(model_repo_t* repo, const char* model_name, size_t* size) {
     if (!repo || !model_name || !size) return NULL;
     
     model_info_t* model = model_repo_get(repo, model_name);
     if (!model) return NULL;
     
     if (!model->is_downloaded) {
         fprintf(stderr, "[ModelRepo] Model not downloaded: %s\n", model_name);
         return NULL;
     }
     
     // Open file
     FILE* fp = fopen(model->local_path, "rb");
     if (!fp) {
         fprintf(stderr, "[ModelRepo] Failed to open: %s\n", model->local_path);
         return NULL;
     }
     
     // Get file size
     fseek(fp, 0, SEEK_END);
     *size = ftell(fp);
     fseek(fp, 0, SEEK_SET);
     
     // Allocate buffer
     uint8_t* buffer = malloc(*size);
     if (!buffer) {
         fclose(fp);
         return NULL;
     }
     
     // Read file
     if (fread(buffer, 1, *size, fp) != *size) {
         fprintf(stderr, "[ModelRepo] Failed to read model\n");
         free(buffer);
         fclose(fp);
         return NULL;
     }
     
     fclose(fp);
     
     printf("[ModelRepo] Loaded %s (%zu bytes)\n", model_name, *size);
     
     return buffer;
 }
 
 // Check if model exists
 bool model_repo_exists(model_repo_t* repo, const char* model_name) {
     model_info_t* model = model_repo_get(repo, model_name);
     return model && model->is_downloaded;
 }
 
 // Get or download model automatically
 uint8_t* model_repo_get_or_download(model_repo_t* repo, const char* model_name, size_t* size) {
     if (!repo || !model_name || !size) return NULL;
     
     // Check if exists
     if (!model_repo_exists(repo, model_name)) {
         if (!repo->auto_download) {
             fprintf(stderr, "[ModelRepo] Model not available and auto-download disabled\n");
             return NULL;
         }
         
         // Download automatically
         printf("[ModelRepo] Auto-downloading %s...\n", model_name);
         if (model_repo_download(repo, model_name) != 0) {
             return NULL;
         }
     }
     
     // Load model
     return model_repo_load(repo, model_name, size);
 }
 
 // Clear cache
 int model_repo_clear_cache(model_repo_t* repo) {
     if (!repo) return -1;
     
     printf("[ModelRepo] Clearing cache...\n");
     
     for (uint32_t i = 0; i < repo->num_models; i++) {
         if (repo->models[i].is_cached) {
             remove(repo->models[i].local_path);
             repo->models[i].is_downloaded = false;
             repo->models[i].is_cached = false;
         }
     }
     
     repo->cache_size_bytes = 0;
     
     printf("[ModelRepo] Cache cleared\n");
     
     return 0;
 }
 
 // Cleanup
 void model_repo_cleanup(model_repo_t* repo) {
     if (!repo) return;
     
     if (repo->models) {
         free(repo->models);
     }
     
     printf("[ModelRepo] Cleaned up\n");
 }