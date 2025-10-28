/**
 * AION OS - GPU Acceleration for AI
 * Multi-backend support: OpenCL, Vulkan, CUDA
 */

 #ifndef GPU_BACKEND_H
 #define GPU_BACKEND_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 // GPU Backend types
 typedef enum {
     GPU_BACKEND_NONE,
     GPU_BACKEND_OPENCL,
     GPU_BACKEND_VULKAN,
     GPU_BACKEND_CUDA,
     GPU_BACKEND_METAL,
     GPU_BACKEND_AUTO
 } gpu_backend_type_t;
 
 // GPU Device info
 typedef struct {
     char name[128];
     char vendor[64];
     uint64_t memory_bytes;
     uint32_t compute_units;
     uint32_t max_work_group_size;
     bool supports_fp16;
     bool supports_int8;
 } gpu_device_info_t;
 
 // GPU Context
 typedef struct {
     gpu_backend_type_t backend;
     void* native_context;  // OpenCL context, Vulkan device, etc.
     void* command_queue;
     
     gpu_device_info_t device_info;
     
     bool initialized;
 } gpu_context_t;
 
 // GPU Buffer
 typedef struct {
     void* handle;
     size_t size_bytes;
     bool is_mapped;
     void* mapped_ptr;
 } gpu_buffer_t;
 
 // Initialize GPU backend
 int gpu_init(gpu_context_t* ctx, gpu_backend_type_t backend);
 
 // Query available devices
 int gpu_list_devices(gpu_device_info_t** devices, uint32_t* count);
 
 // Select device
 int gpu_select_device(gpu_context_t* ctx, uint32_t device_index);
 
 // Buffer operations
 gpu_buffer_t* gpu_buffer_create(gpu_context_t* ctx, size_t size);
 int gpu_buffer_write(gpu_context_t* ctx, gpu_buffer_t* buffer, const void* data, size_t size);
 int gpu_buffer_read(gpu_context_t* ctx, gpu_buffer_t* buffer, void* data, size_t size);
 void gpu_buffer_destroy(gpu_context_t* ctx, gpu_buffer_t* buffer);
 
 // Compute kernel execution
 typedef struct {
     void* program;
     void* kernel;
     char name[64];
 } gpu_kernel_t;
 
 gpu_kernel_t* gpu_kernel_create(gpu_context_t* ctx, const char* source, const char* kernel_name);
 int gpu_kernel_set_arg(gpu_kernel_t* kernel, uint32_t index, void* value, size_t size);
 int gpu_kernel_execute(gpu_context_t* ctx, gpu_kernel_t* kernel, 
                         uint32_t work_dim, const size_t* global_work_size);
 void gpu_kernel_destroy(gpu_kernel_t* kernel);
 
 // Optimized operations for AI
 int gpu_matmul(gpu_context_t* ctx, 
                gpu_buffer_t* A, gpu_buffer_t* B, gpu_buffer_t* C,
                uint32_t M, uint32_t N, uint32_t K);
                
 int gpu_conv2d(gpu_context_t* ctx,
                gpu_buffer_t* input, gpu_buffer_t* kernel, gpu_buffer_t* output,
                uint32_t batch, uint32_t in_channels, uint32_t out_channels,
                uint32_t height, uint32_t width, uint32_t kernel_size);
 
 // Cleanup
 void gpu_cleanup(gpu_context_t* ctx);
 
 #endif // GPU_BACKEND_H