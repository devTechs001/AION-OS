#include "ai_core.h"
#include <math.h>
#include <string.h>

// Activation Functions
static float relu(float x) {
    return (x > 0) ? x : 0;
}

static float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

static float tanh_activation(float x) {
    return tanhf(x);
}

// Create neural network
neural_network_t* ai_create_network(int* layer_sizes, int num_layers) {
    neural_network_t* nn = kmalloc(sizeof(neural_network_t));
    memset(nn, 0, sizeof(neural_network_t));
    
    nn->num_layers = num_layers - 1; // Exclude input layer
    nn->learning_rate = 0.01f;
    
    for (int i = 0; i < nn->num_layers; i++) {
        nn_layer_t* layer = &nn->layers[i];
        layer->input_size = layer_sizes[i];
        layer->output_size = layer_sizes[i + 1];
        
        // Allocate weights and biases
        int weight_count = layer->input_size * layer->output_size;
        layer->weights = kmalloc(weight_count * sizeof(float));
        layer->biases = kmalloc(layer->output_size * sizeof(float));
        
        // Initialize with random values (Xavier initialization)
        float scale = sqrtf(2.0f / layer->input_size);
        for (int j = 0; j < weight_count; j++) {
            layer->weights[j] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * scale;
        }
        for (int j = 0; j < layer->output_size; j++) {
            layer->biases[j] = 0.0f;
        }
        
        // Set activation function
        if (i < nn->num_layers - 1) {
            layer->activation = relu;
        } else {
            layer->activation = sigmoid; // Output layer
        }
    }
    
    return nn;
}

// Forward pass
float* ai_predict(neural_network_t* nn, float* inputs) {
    float* current = inputs;
    float* output = NULL;
    
    for (int i = 0; i < nn->num_layers; i++) {
        nn_layer_t* layer = &nn->layers[i];
        
        output = kmalloc(layer->output_size * sizeof(float));
        
        // Matrix multiplication + bias
        for (int j = 0; j < layer->output_size; j++) {
            float sum = layer->biases[j];
            for (int k = 0; k < layer->input_size; k++) {
                sum += current[k] * layer->weights[k * layer->output_size + j];
            }
            output[j] = layer->activation(sum);
        }
        
        if (i > 0) kfree(current);
        current = output;
    }
    
    return output;
}

// AI: Predict memory allocation size based on historical pattern
uint64_t ai_predict_memory_allocation(process_t* proc) {
    if (!proc) return 4096; // Default page size
    
    // Collect features
    float features[8];
    features[0] = (float)proc->memory_usage / (1024 * 1024); // MB
    features[1] = (float)proc->num_allocations;
    features[2] = (float)proc->avg_allocation_size;
    features[3] = (float)proc->cpu_time / 1000000; // seconds
    features[4] = (float)proc->priority;
    features[5] = (float)(proc->state == PROCESS_RUNNING ? 1 : 0);
    features[6] = (float)proc->num_threads;
    features[7] = (float)proc->page_faults;
    
    // Use pre-trained model (simplified)
    static neural_network_t* mem_model = NULL;
    if (!mem_model) {
        int layers[] = {8, 16, 8, 1};
        mem_model = ai_create_network(layers, 4);
        // In real implementation, load pre-trained weights
    }
    
    float* prediction = ai_predict(mem_model, features);
    uint64_t predicted_size = (uint64_t)(prediction[0] * 1024 * 1024); // Convert to bytes
    
    kfree(prediction);
    
    // Clamp to reasonable range
    if (predicted_size < 4096) predicted_size = 4096;
    if (predicted_size > 1024 * 1024 * 1024) predicted_size = 1024 * 1024 * 1024; // Max 1GB
    
    return predicted_size;
}

// AI: Detect memory leak
bool ai_detect_memory_leak(process_t* proc) {
    if (!proc) return false;
    
    // Simple heuristic: memory growing without bound
    static uint64_t last_memory[1024] = {0};
    static int measurement_count[1024] = {0};
    
    int pid = proc->pid;
    if (pid >= 1024) return false;
    
    uint64_t current_memory = proc->memory_usage;
    
    if (measurement_count[pid] > 10) {
        // Check if memory is consistently increasing
        if (current_memory > last_memory[pid] * 1.5) {
            // Memory increased by 50% - potential leak
            return true;
        }
    }
    
    last_memory[pid] = current_memory;
    measurement_count[pid]++;
    
    return false;
}

// AI: Detect CPU usage anomaly
bool ai_detect_cpu_anomaly(process_t* proc) {
    if (!proc) return false;
    
    // Use statistical anomaly detection
    static float cpu_history[1024][100] = {0};
    static int history_index[1024] = {0};
    
    int pid = proc->pid;
    if (pid >= 1024) return false;
    
    float current_cpu = (float)proc->cpu_usage_percent;
    
    // Add to history
    int idx = history_index[pid];
    cpu_history[pid][idx] = current_cpu;
    history_index[pid] = (idx + 1) % 100;
    
    // Calculate mean and standard deviation
    float sum = 0.0f;
    for (int i = 0; i < 100; i++) {
        sum += cpu_history[pid][i];
    }
    float mean = sum / 100.0f;
    
    float variance = 0.0f;
    for (int i = 0; i < 100; i++) {
        float diff = cpu_history[pid][i] - mean;
        variance += diff * diff;
    }
    float stddev = sqrtf(variance / 100.0f);
    
    // Anomaly if > 3 standard deviations from mean
    if (fabsf(current_cpu - mean) > 3.0f * stddev) {
        return true;
    }
    
    return false;
}