/**
 * AION OS - On-Device Training System
 * Enables continuous learning and model adaptation
 */

 #ifndef TRAINER_H
 #define TRAINER_H
 
 #include "../runtime/embedded_tflite.h"
 #include <stdint.h>
 
 // Training configuration
 typedef struct {
     float learning_rate;
     uint32_t batch_size;
     uint32_t num_epochs;
     
     enum {
         OPTIMIZER_SGD,
         OPTIMIZER_ADAM,
         OPTIMIZER_RMSPROP
     } optimizer;
     
     enum {
         LOSS_MSE,
         LOSS_CROSS_ENTROPY,
         LOSS_BINARY_CROSS_ENTROPY
     } loss_function;
     
     bool use_gpu;
     bool use_mixed_precision;
     
     // Regularization
     float l2_regularization;
     float dropout_rate;
     
 } training_config_t;
 
 // Training dataset
 typedef struct {
     float** inputs;
     float** labels;
     uint32_t num_samples;
     uint32_t input_size;
     uint32_t output_size;
 } dataset_t;
 
 // Training context
 typedef struct {
     aion_model_t* model;
     training_config_t config;
     
     // Optimizer state
     float** gradients;
     float** momentum;
     float** velocity;
     
     // Training stats
     uint32_t current_epoch;
     float current_loss;
     float* loss_history;
     
     // GPU context (if enabled)
     void* gpu_context;
     
     bool is_training;
 } trainer_t;
 
 // Initialize trainer
 int trainer_init(trainer_t* trainer, aion_model_t* model, training_config_t* config);
 
 // Train model
 int trainer_train(trainer_t* trainer, dataset_t* train_data, dataset_t* val_data);
 
 // Single training step
 float trainer_step(trainer_t* trainer, float* input, float* label);
 
 // Evaluate model
 float trainer_evaluate(trainer_t* trainer, dataset_t* test_data);
 
 // Fine-tune pre-trained model
 int trainer_fine_tune(trainer_t* trainer, dataset_t* data, uint32_t num_frozen_layers);
 
 // Transfer learning
 int trainer_transfer_learn(trainer_t* trainer, aion_model_t* pretrained,
                            dataset_t* new_task_data);
 
 // Federated learning (multi-device)
 int trainer_federated_update(trainer_t* trainer, float** gradients_from_devices,
                              uint32_t num_devices);
 
 // Save checkpoint
 int trainer_save_checkpoint(trainer_t* trainer, const char* path);
 
 // Load checkpoint
 int trainer_load_checkpoint(trainer_t* trainer, const char* path);
 
 // Cleanup
 void trainer_cleanup(trainer_t* trainer);
 
 #endif // TRAINER_H