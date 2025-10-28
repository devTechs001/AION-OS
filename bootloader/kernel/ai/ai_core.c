#ifndef AI_CORE_H
#define AI_CORE_H

#include <stdint.h>
#include <stdbool.h>

// Neural Network Layer
typedef struct {
    float* weights;
    float* biases;
    int input_size;
    int output_size;
    
    // Activation function
    float (*activation)(float);
} nn_layer_t;

// Neural Network
typedef struct {
    nn_layer_t layers[16];
    int num_layers;
    
    float learning_rate;
    
    // Training data
    float* training_inputs;
    float* training_outputs;
    int training_samples;
} neural_network_t;

// AI Prediction Context
typedef struct {
    // Memory Prediction
    struct {
        uint64_t predicted_allocation_size;
        uint64_t predicted_deallocation_time;
        float fragmentation_probability;
    } memory;
    
    // CPU Prediction
    struct {
        uint32_t predicted_cpu_usage;
        uint64_t predicted_context_switches;
        int recommended_cpu_affinity;
    } cpu;
    
    // I/O Prediction
    struct {
        uint64_t predicted_read_size;
        uint64_t predicted_write_size;
        uint64_t predicted_latency_us;
        bool sequential_access;
    } io;
    
    // Network Prediction
    struct {
        uint32_t predicted_bandwidth_mbps;
        uint32_t predicted_latency_ms;
        float packet_loss_probability;
    } network;
} ai_prediction_t;

// AI System Stats
typedef struct {
    uint64_t predictions_made;
    uint64_t predictions_accurate;
    float accuracy_rate;
    
    uint64_t training_iterations;
    float current_loss;
    
    uint64_t inference_time_total_us;
    uint32_t avg_inference_time_us;
} ai_stats_t;

// Function Prototypes
void ai_core_init(void);

// Neural Network Operations
neural_network_t* ai_create_network(int* layer_sizes, int num_layers);
void ai_train_network(neural_network_t* nn, float* inputs, float* outputs, int epochs);
float* ai_predict(neural_network_t* nn, float* inputs);
void ai_destroy_network(neural_network_t* nn);

// System Predictions
ai_prediction_t* ai_predict_system_behavior(void);
uint64_t ai_predict_memory_allocation(process_t* proc);
uint32_t ai_predict_cpu_usage(process_t* proc, uint64_t future_ms);
uint64_t ai_predict_io_latency(int device_id, uint64_t offset, size_t size);

// Anomaly Detection
bool ai_detect_memory_leak(process_t* proc);
bool ai_detect_cpu_anomaly(process_t* proc);
bool ai_detect_security_threat(const char* event);

// Optimization
void ai_optimize_memory_layout(void);
void ai_optimize_scheduler_quantum(void);
void ai_optimize_cache_prefetch(void);

#endif // AI_CORE_H