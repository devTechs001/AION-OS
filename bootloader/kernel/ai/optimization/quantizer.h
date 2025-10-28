/**
 * AION OS - Model Quantization & Optimization
 * Reduces model size and improves inference speed
 */

 #ifndef QUANTIZER_H
 #define QUANTIZER_H
 
 #include "../runtime/embedded_tflite.h"
 #include <stdint.h>
 
 // Quantization types
 typedef enum {
     QUANT_NONE,           // FP32 (no quantization)
     QUANT_DYNAMIC,        // Dynamic range quantization
     QUANT_INT8,           // Full INT8 quantization
     QUANT_FLOAT16,        // FP16 quantization
     QUANT_MIXED           // Mixed precision
 } quantization_type_t;
 
 // Quantization configuration
 typedef struct {
     quantization_type_t type;
     
     // Calibration dataset
     float** calibration_data;
     uint32_t num_samples;
     
     // Optimization options
     bool optimize_for_size;
     bool optimize_for_latency;
     bool allow_fp16;
     
     // Target hardware
     bool has_int8_accelerator;
     bool has_fp16_accelerator;
 } quant_config_t;
 
 // Quantized model
 typedef struct {
     aion_model_t* model;
     
     quantization_type_t quant_type;
     float compression_ratio;
     
     // Calibration statistics
     float* min_values;
     float* max_values;
     float* scales;
     int32_t* zero_points;
 } quantized_model_t;
 
 // Initialize quantizer
 int quantizer_init();
 
 // Quantize model
 quantized_model_t* quantizer_quantize(aion_model_t* model, quant_config_t* config);
 
 // Dynamic quantization (no calibration needed)
 quantized_model_t* quantizer_dynamic_quant(aion_model_t* model);
 
 // Post-training quantization with calibration
 quantized_model_t* quantizer_ptq_int8(aion_model_t* model, quant_config_t* config);
 
 // FP32 to FP16 conversion
 quantized_model_t* quantizer_fp16(aion_model_t* model);
 
 // Model pruning (remove unnecessary weights)
 int quantizer_prune(aion_model_t* model, float threshold);
 
 // Knowledge distillation (compress large model to smaller one)
 aion_model_t* quantizer_distill(aion_model_t* teacher, aion_model_t* student,
                                 float** training_data, uint32_t num_samples);
 
 // Operator fusion optimization
 int quantizer_fuse_ops(aion_model_t* model);
 
 // Save quantized model
 int quantizer_save(quantized_model_t* model, const char* path);
 
 // Cleanup
 void quantizer_cleanup(quantized_model_t* model);
 
 #endif // QUANTIZER_H