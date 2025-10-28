#ifndef TFLITE_H
#define TFLITE_H

#include <stdint.h>
#include <stdbool.h>

// TensorFlow Lite Tensor Types
#define TFLITE_FLOAT32      0
#define TFLITE_INT32        1
#define TFLITE_UINT8        2
#define TFLITE_INT64        3
#define TFLITE_STRING       4
#define TFLITE_BOOL         5
#define TFLITE_INT16        6
#define TFLITE_COMPLEX64    7
#define TFLITE_INT8         8
#define TFLITE_FLOAT16      9

// Quantization Parameters
typedef struct {
    float scale;
    int32_t zero_point;
    int quantized_dimension;
} tflite_quantization_t;

// Tensor
typedef struct {
    void* data;
    int* dims;
    int num_dims;
    int type;
    size_t bytes;
    
    char name[64];
    
    tflite_quantization_t quantization;
    bool is_quantized;
} tflite_tensor_t;

// Operator/Node
typedef struct {
    int opcode;
    char op_name[32];
    
    int* inputs;
    int num_inputs;
    
    int* outputs;
    int num_outputs;
    
    void* builtin_options;
} tflite_operator_t;

// Subgraph
typedef struct {
    tflite_tensor_t* tensors;
    int num_tensors;
    
    tflite_operator_t* operators;
    int num_operators;
    
    int* inputs;
    int num_inputs;
    
    int* outputs;
    int num_outputs;
} tflite_subgraph_t;

// TFLite Model
typedef struct {
    char model_path[256];
    
    // Model version
    uint32_t version;
    
    // Subgraphs (typically 1 for simple models)
    tflite_subgraph_t* subgraphs;
    int num_subgraphs;
    
    // Buffers (weights)
    void** buffers;
    size_t* buffer_sizes;
    int num_buffers;
    
    // Metadata
    char description[256];
    
    bool loaded;
    size_t total_size;
} tflite_model_t;

// Interpreter (Execution Engine)
typedef struct {
    tflite_model_t* model;
    
    // Current subgraph being executed
    int current_subgraph;
    
    // Execution context
    tflite_tensor_t** input_tensors;
    int num_input_tensors;
    
    tflite_tensor_t** output_tensors;
    int num_output_tensors;
    
    // Execution options
    int num_threads;
    bool use_gpu;
    bool use_nnapi;      // Neural Network API (Android)
    bool allow_fp16;     // Half-precision floats
    
    // Memory arena for intermediate tensors
    void* arena;
    size_t arena_size;
    size_t arena_used;
    
    // Statistics
    uint64_t invocations;
    uint64_t total_time_us;
    uint32_t avg_time_us;
    
    spinlock_t lock;
} tflite_interpreter_t;

// Built-in Operators
typedef enum {
    TFLITE_BUILTIN_ADD = 0,
    TFLITE_BUILTIN_CONV_2D = 3,
    TFLITE_BUILTIN_DEPTHWISE_CONV_2D = 4,
    TFLITE_BUILTIN_FULLY_CONNECTED = 9,
    TFLITE_BUILTIN_SOFTMAX = 25,
    TFLITE_BUILTIN_RESHAPE = 22,
    TFLITE_BUILTIN_RELU = 19,
    TFLITE_BUILTIN_AVERAGE_POOL_2D = 1,
    TFLITE_BUILTIN_MAX_POOL_2D = 17,
    TFLITE_BUILTIN_CONCATENATION = 2,
    // Add more as needed
} tflite_builtin_operator;

// Function Prototypes
void tflite_init(void);

// Model Management
tflite_model_t* tflite_load_model(const char* model_path);
void tflite_unload_model(tflite_model_t* model);
void tflite_print_model_info(tflite_model_t* model);

// Interpreter Management
tflite_interpreter_t* tflite_create_interpreter(tflite_model_t* model);
void tflite_destroy_interpreter(tflite_interpreter_t* interpreter);

// Configuration
void tflite_set_num_threads(tflite_interpreter_t* interpreter, int num_threads);
void tflite_use_gpu(tflite_interpreter_t* interpreter, bool enable);
int tflite_allocate_tensors(tflite_interpreter_t* interpreter);

// Input/Output
tflite_tensor_t* tflite_get_input_tensor(tflite_interpreter_t* interpreter, int index);
tflite_tensor_t* tflite_get_output_tensor(tflite_interpreter_t* interpreter, int index);
void tflite_set_tensor_data(tflite_tensor_t* tensor, void* data, size_t size);
void* tflite_get_tensor_data(tflite_tensor_t* tensor);

// Execution
int tflite_invoke(tflite_interpreter_t* interpreter);

// Quantization
void tflite_quantize_tensor(tflite_tensor_t* tensor, float* float_data);
void tflite_dequantize_tensor(tflite_tensor_t* tensor, float* output);

// Optimization
void tflite_optimize_model(tflite_model_t* model);
void tflite_convert_to_fp16(tflite_model_t* model);

#endif // TFLITE_H