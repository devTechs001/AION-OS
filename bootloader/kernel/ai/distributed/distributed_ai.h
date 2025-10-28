/**
 * AION OS - Distributed AI System
 * Multi-device AI coordination and workload distribution
 */

 #ifndef DISTRIBUTED_AI_H
 #define DISTRIBUTED_AI_H
 
 #include "../runtime/embedded_tflite.h"
 #include "../training/trainer.h"
 #include <stdint.h>
 #include <stdbool.h>
 
 // Device types in distributed system
 typedef enum {
     DEVICE_TYPE_DESKTOP,
     DEVICE_TYPE_LAPTOP,
     DEVICE_TYPE_MOBILE,
     DEVICE_TYPE_EDGE,
     DEVICE_TYPE_CLOUD
 } device_type_t;
 
 // Device capabilities
 typedef struct {
     char device_id[64];
     char hostname[128];
     device_type_t type;
     
     // Hardware specs
     uint32_t num_cpu_cores;
     uint64_t ram_bytes;
     bool has_gpu;
     uint64_t gpu_memory_bytes;
     
     // Network
     char ip_address[64];
     uint32_t port;
     uint32_t bandwidth_mbps;
     uint32_t latency_ms;
     
     // AI capabilities
     bool supports_training;
     bool supports_inference;
     float compute_power;  // Relative performance score
     
     // Status
     bool is_online;
     float cpu_usage;
     float memory_usage;
     uint32_t active_tasks;
 } device_info_t;
 
 // Distributed task
 typedef struct {
     char task_id[64];
     
     enum {
         TASK_INFERENCE,
         TASK_TRAINING,
         TASK_MODEL_SYNC,
         TASK_DATA_TRANSFER
     } type;
     
     void* model_data;
     size_t model_size;
     
     void* input_data;
     size_t input_size;
     
     void* output_data;
     size_t output_size;
     
     char assigned_device[64];
     
     enum {
         TASK_PENDING,
         TASK_RUNNING,
         TASK_COMPLETED,
         TASK_FAILED
     } status;
     
     uint64_t start_time;
     uint64_t end_time;
     
 } distributed_task_t;
 
 // Distributed AI context
 typedef struct {
     device_info_t* devices;
     uint32_t num_devices;
     
     device_info_t local_device;
     
     distributed_task_t* tasks;
     uint32_t num_tasks;
     
     // Model sharding
     aion_model_t** model_shards;
     uint32_t num_shards;
     
     // Communication
     int server_socket;
     bool is_coordinator;
     
     // Load balancing
     float* device_loads;
     
     // Federated learning state
     bool federated_mode;
     uint32_t federated_round;
     
 } distributed_ai_t;
 
 // Initialize distributed AI
 int distributed_ai_init(distributed_ai_t* dist, bool is_coordinator);
 
 // Register device
 int distributed_ai_register_device(distributed_ai_t* dist, device_info_t* device);
 
 // Discover devices on network
 int distributed_ai_discover_devices(distributed_ai_t* dist);
 
 // Distribute inference task
 int distributed_ai_inference(distributed_ai_t* dist, aion_model_t* model,
                             float* input, float* output);
 
 // Distribute training task (data parallelism)
 int distributed_ai_train_data_parallel(distributed_ai_t* dist, trainer_t* trainer,
                                       dataset_t* dataset);
 
 // Distribute training task (model parallelism)
 int distributed_ai_train_model_parallel(distributed_ai_t* dist, trainer_t* trainer,
                                        dataset_t* dataset);
 
 // Federated learning
 int distributed_ai_federated_train(distributed_ai_t* dist, aion_model_t* model,
                                   dataset_t* local_dataset, uint32_t num_rounds);
 
 // Model sharding (split large model across devices)
 int distributed_ai_shard_model(distributed_ai_t* dist, aion_model_t* model,
                               uint32_t num_shards);
 
 // Synchronize models across devices
 int distributed_ai_sync_model(distributed_ai_t* dist, aion_model_t* model);
 
 // Load balancing
 device_info_t* distributed_ai_select_device(distributed_ai_t* dist, distributed_task_t* task);
 
 // Monitor device health
 int distributed_ai_monitor(distributed_ai_t* dist);
 
 // Cleanup
 void distributed_ai_cleanup(distributed_ai_t* dist);
 
 #endif // DISTRIBUTED_AI_H