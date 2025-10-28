/**
 * AION OS - REAL TensorFlow Lite Integration
 * Using actual TensorFlow Lite C API
 */

 #ifndef TFLITE_REAL_H
 #define TFLITE_REAL_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 // Include actual TensorFlow Lite C headers
 // These would come from: https://github.com/tensorflow/tensorflow/tree/master/tensorflow/lite/c
 #include "tensorflow/lite/c/c_api.h"
 #include "tensorflow/lite/c/common.h"
 
 // AION OS wrapper around TFLite
 typedef struct {
     TfLiteModel* model;
     TfLiteInterpreter* interpreter;
     TfLiteInterpreterOptions* options;
     
     // Tensor info
     TfLiteTensor* input_tensor;
     const TfLiteTensor* output_tensor;
     
     // Performance
     bool use_gpu;
     bool use_nnapi;
     uint32_t num_threads;
     
     // Cache
     void* model_cache;
     size_t cache_size;
 } aion_tflite_t;
 
 // Initialize TFLite runtime
 int aion_tflite_init(aion_tflite_t* ctx, uint32_t num_threads);
 
 // Load model from file or memory
 int aion_tflite_load_model_file(aion_tflite_t* ctx, const char* model_path);
 int aion_tflite_load_model_buffer(aion_tflite_t* ctx, const void* model_data, size_t size);
 
 // Allocate tensors
 int aion_tflite_allocate_tensors(aion_tflite_t* ctx);
 
 // Get tensor info
 TfLiteTensor* aion_tflite_get_input_tensor(aion_tflite_t* ctx, int index);
 const TfLiteTensor* aion_tflite_get_output_tensor(aion_tflite_t* ctx, int index);
 
 // Run inference
 int aion_tflite_invoke(aion_tflite_t* ctx);
 
 // Helpers for common types
 int aion_tflite_set_input_float(aion_tflite_t* ctx, int index, const float* data, size_t size);
 int aion_tflite_get_output_float(aion_tflite_t* ctx, int index, float* data, size_t size);
 
 // Cleanup
 void aion_tflite_cleanup(aion_tflite_t* ctx);
 
 // Enable GPU acceleration (if available)
 int aion_tflite_use_gpu(aion_tflite_t* ctx);
 
 // Enable XNNPACK delegate (CPU optimization)
 int aion_tflite_use_xnnpack(aion_tflite_t* ctx);
 
 #endif // TFLITE_REAL_H