/**
 * AION OS - Quantizer Implementation
 */

 #include "quantizer.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 
 // Initialize quantizer
 int quantizer_init() {
     printf("[Quantizer] Initialized\n");
     return 0;
 }
 
 // Dynamic quantization (weights only)
 quantized_model_t* quantizer_dynamic_quant(aion_model_t* model) {
     if (!model) return NULL;
     
     printf("[Quantizer] Applying dynamic quantization...\n");
     
     quantized_model_t* quant_model = calloc(1, sizeof(quantized_model_t));
     if (!quant_model) return NULL;
     
     quant_model->model = model;
     quant_model->quant_type = QUANT_DYNAMIC;
     
     // Quantize weights to INT8
     for (uint32_t i = 0; i < model->num_tensors; i++) {
         aion_tensor_t* tensor = &model->tensors[i];
         
         if (tensor->type != TYPE_FLOAT32) continue;
         
         float* data = (float*)tensor->data;
         size_t num_elements = tensor->bytes / sizeof(float);
         
         // Find min/max
         float min_val = data[0];
         float max_val = data[0];
         
         for (size_t j = 1; j < num_elements; j++) {
             if (data[j] < min_val) min_val = data[j];
             if (data[j] > max_val) max_val = data[j];
         }
         
         // Calculate scale and zero point
         float scale = (max_val - min_val) / 255.0f;
         int32_t zero_point = (int32_t)(-min_val / scale);
         
         // Quantize to INT8
         int8_t* quantized = malloc(num_elements);
         for (size_t j = 0; j < num_elements; j++) {
             float scaled = data[j] / scale + zero_point;
             quantized[j] = (int8_t)fminf(127.0f, fmaxf(-128.0f, roundf(scaled)));
         }
         
         // Store quantization parameters
         tensor->scale = scale;
         tensor->zero_point = zero_point;
         tensor->is_quantized = true;
         
         // Replace data
         free(tensor->data);
         tensor->data = quantized;
         tensor->type = TYPE_INT8;
         tensor->bytes = num_elements;
     }
     
     // Calculate compression ratio
     quant_model->compression_ratio = 4.0f;  // FP32 to INT8 = 4x compression
     
     printf("[Quantizer] Dynamic quantization complete (%.1fx compression)\n",
            quant_model->compression_ratio);
     
     return quant_model;
 }
 
 // Post-training quantization (INT8)
 quantized_model_t* quantizer_ptq_int8(aion_model_t* model, quant_config_t* config) {
     if (!model || !config) return NULL;
     
     printf("[Quantizer] Applying INT8 post-training quantization...\n");
     printf("[Quantizer] Calibration samples: %u\n", config->num_samples);
     
     quantized_model_t* quant_model = calloc(1, sizeof(quantized_model_t));
     if (!quant_model) return NULL;
     
     quant_model->model = model;
     quant_model->quant_type = QUANT_INT8;
     
     // Allocate calibration statistics
     quant_model->min_values = calloc(model->num_tensors, sizeof(float));
     quant_model->max_values = calloc(model->num_tensors, sizeof(float));
     quant_model->scales = calloc(model->num_tensors, sizeof(float));
     quant_model->zero_points = calloc(model->num_tensors, sizeof(int32_t));
     
     // Run calibration
     printf("[Quantizer] Running calibration...\n");
     
     for (uint32_t sample = 0; sample < config->num_samples; sample++) {
         // Run inference with calibration data
         // TODO: Actually run model inference
         
         // Collect activation statistics
         for (uint32_t i = 0; i < model->num_tensors; i++) {
             aion_tensor_t* tensor = &model->tensors[i];
             
             if (tensor->type != TYPE_FLOAT32) continue;
             
             float* data = (float*)tensor->data;
             size_t num_elements = tensor->bytes / sizeof(float);
             
             // Update min/max
             for (size_t j = 0; j < num_elements; j++) {
                 if (sample == 0 && j == 0) {
                     quant_model->min_values[i] = data[j];
                     quant_model->max_values[i] = data[j];
                 } else {
                     if (data[j] < quant_model->min_values[i])
                         quant_model->min_values[i] = data[j];
                     if (data[j] > quant_model->max_values[i])
                         quant_model->max_values[i] = data[j];
                 }
             }
         }
     }
     
     // Quantize based on calibration
     for (uint32_t i = 0; i < model->num_tensors; i++) {
         aion_tensor_t* tensor = &model->tensors[i];
         
         if (tensor->type != TYPE_FLOAT32) continue;
         
         // Calculate quantization parameters
         float min = quant_model->min_values[i];
         float max = quant_model->max_values[i];
         
         float scale = (max - min) / 255.0f;
         int32_t zero_point = (int32_t)(-min / scale);
         
         quant_model->scales[i] = scale;
         quant_model->zero_points[i] = zero_point;
         
         // Quantize tensor
         float* data = (float*)tensor->data;
         size_t num_elements = tensor->bytes / sizeof(float);
         
         int8_t* quantized = malloc(num_elements);
         for (size_t j = 0; j < num_elements; j++) {
             float scaled = data[j] / scale + zero_point;
             quantized[j] = (int8_t)fminf(127.0f, fmaxf(-128.0f, roundf(scaled)));
         }
         
         tensor->scale = scale;
         tensor->zero_point = zero_point;
         tensor->is_quantized = true;
         
         free(tensor->data);
         tensor->data = quantized;
         tensor->type = TYPE_INT8;
         tensor->bytes = num_elements;
     }
     
     quant_model->compression_ratio = 4.0f;
     
     printf("[Quantizer] INT8 quantization complete\n");
     
     return quant_model;
 }
 
 // FP16 quantization
 quantized_model_t* quantizer_fp16(aion_model_t* model) {
     if (!model) return NULL;
     
     printf("[Quantizer] Applying FP16 quantization...\n");
     
     quantized_model_t* quant_model = calloc(1, sizeof(quantized_model_t));
     if (!quant_model) return NULL;
     
     quant_model->model = model;
     quant_model->quant_type = QUANT_FLOAT16;
     
     // Convert FP32 to FP16
     for (uint32_t i = 0; i < model->num_tensors; i++) {
         aion_tensor_t* tensor = &model->tensors[i];
         
         if (tensor->type != TYPE_FLOAT32) continue;
         
         float* fp32_data = (float*)tensor->data;
         size_t num_elements = tensor->bytes / sizeof(float);
         
         // Allocate FP16 buffer (using uint16_t as placeholder)
         uint16_t* fp16_data = malloc(num_elements * sizeof(uint16_t));
         
         // Convert each element
         for (size_t j = 0; j < num_elements; j++) {
             // Simplified FP32 to FP16 conversion
             fp16_data[j] = float_to_fp16(fp32_data[j]);
         }
         
         free(tensor->data);
         tensor->data = fp16_data;
         tensor->bytes = num_elements * sizeof(uint16_t);
     }
     
     quant_model->compression_ratio = 2.0f;  // FP32 to FP16 = 2x compression
     
     printf("[Quantizer] FP16 quantization complete (2x compression)\n");
     
     return quant_model;
 }
 
 // Helper: FP32 to FP16 conversion
 static uint16_t float_to_fp16(float value) {
     // Simplified conversion (proper implementation would use IEEE 754 format)
     uint32_t bits = *(uint32_t*)&value;
     
     uint16_t sign = (bits >> 16) & 0x8000;
     uint16_t exponent = ((bits >> 23) & 0xFF) - 112;
     uint16_t mantissa = (bits >> 13) & 0x3FF;
     
     if (exponent <= 0) return sign;  // Underflow
     if (exponent >= 31) return sign | 0x7C00;  // Overflow/infinity
     
     return sign | (exponent << 10) | mantissa;
 }
 
 // Model pruning
 int quantizer_prune(aion_model_t* model, float threshold) {
     if (!model) return -1;
     
     printf("[Quantizer] Pruning weights with threshold %.4f...\n", threshold);
     
     uint32_t total_params = 0;
     uint32_t pruned_params = 0;
     
     for (uint32_t i = 0; i < model->num_tensors; i++) {
         aion_tensor_t* tensor = &model->tensors[i];
         
         if (tensor->type != TYPE_FLOAT32) continue;
         
         float* data = (float*)tensor->data;
         size_t num_elements = tensor->bytes / sizeof(float);
         
         total_params += num_elements;
         
         // Zero out small weights
         for (size_t j = 0; j < num_elements; j++) {
             if (fabsf(data[j]) < threshold) {
                 data[j] = 0.0f;
                 pruned_params++;
             }
         }
     }
     
     float sparsity = (float)pruned_params / total_params * 100.0f;
     
     printf("[Quantizer] Pruned %u/%u parameters (%.1f%% sparsity)\n",
            pruned_params, total_params, sparsity);
     
     return 0;
 }
 
 // Operator fusion
 int quantizer_fuse_ops(aion_model_t* model) {
     if (!model) return -1;
     
     printf("[Quantizer] Fusing operators...\n");
     
     uint32_t fused_count = 0;
     
     // Look for common patterns to fuse
     for (uint32_t i = 0; i < model->num_operators - 1; i++) {
         aion_op_t* op1 = &model->operators[i];
         aion_op_t* op2 = &model->operators[i + 1];
         
         // Conv2D + ReLU fusion
         if (op1->type == OP_CONV_2D && op2->type == OP_RELU) {
             printf("[Quantizer] Fused Conv2D + ReLU\n");
             fused_count++;
         }
         
         // FullyConnected + ReLU fusion
         if (op1->type == OP_FULLY_CONNECTED && op2->type == OP_RELU) {
             printf("[Quantizer] Fused FC + ReLU\n");
             fused_count++;
         }
     }
     
     printf("[Quantizer] Fused %u operators\n", fused_count);
     
     return 0;
 }
 
 // Cleanup
 void quantizer_cleanup(quantized_model_t* model) {
     if (!model) return;
     
     if (model->min_values) free(model->min_values);
     if (model->max_values) free(model->max_values);
     if (model->scales) free(model->scales);
     if (model->zero_points) free(model->zero_points);
     
     free(model);
     
     printf("[Quantizer] Cleaned up\n");
 }