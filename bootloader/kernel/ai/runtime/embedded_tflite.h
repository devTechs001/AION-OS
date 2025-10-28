/**
 * AION OS - Embedded TensorFlow Lite Runtime
 * Self-contained, no external dependencies
 * Compiled directly into kernel
 */

 #ifndef EMBEDDED_TFLITE_H
 #define EMBEDDED_TFLITE_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 // Embedded TFLite - Minimal footprint version
 // Based on TFLite Micro (https://github.com/tensorflow/tflite-micro)
 
 // We'll implement a minimal TFLite interpreter from scratch
 // This eliminates the need for external TFLite library
 
 // Op Kernel Types
 typedef enum {
     OP_CONV_2D,
     OP_DEPTHWISE_CONV_2D,
     OP_FULLY_CONNECTED,
     OP_POOLING,
     OP_SOFTMAX,
     OP_ADD,
     OP_MUL,
     OP_RESHAPE,
     OP_RELU,
     OP_SIGMOID,
     OP_TANH,
     OP_QUANTIZE,
     OP_DEQUANTIZE,
     OP_MAX
 } op_type_t;
 
 // Tensor
 typedef struct {
     void* data;
     int32_t dims[8];
     uint32_t num_dims;
     uint32_t bytes;
     
     enum {
         TYPE_FLOAT32,
         TYPE_INT8,
         TYPE_UINT8,
         TYPE_INT32,
         TYPE_INT64
     } type;
     
     // Quantization params
     float scale;
     int32_t zero_point;
     bool is_quantized;
 } aion_tensor_t;
 
 // Operator
 typedef struct {
     op_type_t type;
     void* params;
     uint32_t* inputs;
     uint32_t num_inputs;
     uint32_t* outputs;
     uint32_t num_outputs;
 } aion_op_t;
 
 // Embedded Model
 typedef struct {
     aion_tensor_t* tensors;
     uint32_t num_tensors;
     
     aion_op_t* operators;
     uint32_t num_operators;
     
     uint32_t* input_indices;
     uint32_t num_inputs;
     
     uint32_t* output_indices;
     uint32_t num_outputs;
     
     void* arena;  // Memory arena for tensors
     size_t arena_size;
 } aion_model_t;
 
 // Interpreter
 typedef struct {
     aion_model_t* model;
     
     // Execution backend
     enum {
         BACKEND_CPU,
         BACKEND_GPU_OPENCL,
         BACKEND_GPU_VULKAN,
         BACKEND_GPU_CUDA,
         BACKEND_NPU,
         BACKEND_AUTO
     } backend;
     
     // Performance
     uint32_t num_threads;
     bool use_xnnpack;
     bool use_gpu;
     
     // Accelerator handle
     void* accelerator_context;
     
 } aion_interpreter_t;
 
 // Initialize interpreter
 int aion_interpreter_init(aion_interpreter_t* interp, size_t arena_size);
 
 // Load model from FlatBuffer
 int aion_model_load_flatbuffer(aion_interpreter_t* interp, const uint8_t* buffer, size_t size);
 
 // Load model from ONNX
 int aion_model_load_onnx(aion_interpreter_t* interp, const uint8_t* buffer, size_t size);
 
 // Load model from PyTorch
 int aion_model_load_pytorch(aion_interpreter_t* interp, const uint8_t* buffer, size_t size);
 
 // Allocate tensors
 int aion_interpreter_allocate(aion_interpreter_t* interp);
 
 // Set input
 int aion_interpreter_set_input(aion_interpreter_t* interp, uint32_t index, 
                                 const void* data, size_t bytes);
 
 // Invoke (run inference)
 int aion_interpreter_invoke(aion_interpreter_t* interp);
 
 // Get output
 int aion_interpreter_get_output(aion_interpreter_t* interp, uint32_t index,
                                  void* data, size_t bytes);
 
 // Cleanup
 void aion_interpreter_cleanup(aion_interpreter_t* interp);
 
 // Backend selection
 int aion_interpreter_use_gpu(aion_interpreter_t* interp);
 int aion_interpreter_use_npu(aion_interpreter_t* interp);
 int aion_interpreter_use_cpu(aion_interpreter_t* interp);
 
 #endif // EMBEDDED_TFLITE_H