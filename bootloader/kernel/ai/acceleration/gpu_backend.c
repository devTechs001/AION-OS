/**
 * AION OS - GPU Acceleration Implementation
 * OpenCL backend (most portable across GPUs)
 */

 #include "gpu_backend.h"
 #include <CL/cl.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 // OpenCL context wrapper
 typedef struct {
     cl_platform_id platform;
     cl_device_id device;
     cl_context context;
     cl_command_queue queue;
 } opencl_context_t;
 
 // Initialize GPU backend
 int gpu_init(gpu_context_t* ctx, gpu_backend_type_t backend) {
     if (!ctx) return -1;
     
     memset(ctx, 0, sizeof(gpu_context_t));
     
     // Auto-detect backend if requested
     if (backend == GPU_BACKEND_AUTO) {
         // Try in order: OpenCL > Vulkan > CUDA
         backend = GPU_BACKEND_OPENCL;
     }
     
     ctx->backend = backend;
     
     switch (backend) {
         case GPU_BACKEND_OPENCL: {
             return gpu_init_opencl(ctx);
         }
         
         case GPU_BACKEND_VULKAN: {
             return gpu_init_vulkan(ctx);
         }
         
         case GPU_BACKEND_CUDA: {
             return gpu_init_cuda(ctx);
         }
         
         default:
             fprintf(stderr, "[GPU] Unsupported backend: %d\n", backend);
             return -1;
     }
 }
 
 // Initialize OpenCL
 static int gpu_init_opencl(gpu_context_t* ctx) {
     cl_int err;
     
     // Allocate OpenCL context
     opencl_context_t* ocl = calloc(1, sizeof(opencl_context_t));
     if (!ocl) return -1;
     
     // Get platform
     cl_uint num_platforms;
     err = clGetPlatformIDs(1, &ocl->platform, &num_platforms);
     if (err != CL_SUCCESS || num_platforms == 0) {
         fprintf(stderr, "[GPU] No OpenCL platforms found\n");
         free(ocl);
         return -1;
     }
     
     // Get GPU device (prefer GPU over CPU)
     err = clGetDeviceIDs(ocl->platform, CL_DEVICE_TYPE_GPU, 1, &ocl->device, NULL);
     if (err != CL_SUCCESS) {
         // Fallback to CPU
         fprintf(stderr, "[GPU] No GPU found, trying CPU...\n");
         err = clGetDeviceIDs(ocl->platform, CL_DEVICE_TYPE_CPU, 1, &ocl->device, NULL);
         if (err != CL_SUCCESS) {
             fprintf(stderr, "[GPU] No OpenCL devices found\n");
             free(ocl);
             return -1;
         }
     }
     
     // Get device info
     char device_name[128];
     char vendor_name[64];
     cl_ulong mem_size;
     cl_uint compute_units;
     size_t max_work_group;
     
     clGetDeviceInfo(ocl->device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
     clGetDeviceInfo(ocl->device, CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, NULL);
     clGetDeviceInfo(ocl->device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(mem_size), &mem_size, NULL);
     clGetDeviceInfo(ocl->device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
     clGetDeviceInfo(ocl->device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group), &max_work_group, NULL);
     
     strncpy(ctx->device_info.name, device_name, sizeof(ctx->device_info.name) - 1);
     strncpy(ctx->device_info.vendor, vendor_name, sizeof(ctx->device_info.vendor) - 1);
     ctx->device_info.memory_bytes = mem_size;
     ctx->device_info.compute_units = compute_units;
     ctx->device_info.max_work_group_size = max_work_group;
     
     // Check FP16 support
     char extensions[4096];
     clGetDeviceInfo(ocl->device, CL_DEVICE_EXTENSIONS, sizeof(extensions), extensions, NULL);
     ctx->device_info.supports_fp16 = strstr(extensions, "cl_khr_fp16") != NULL;
     
     printf("[GPU] Device: %s\n", device_name);
     printf("[GPU] Vendor: %s\n", vendor_name);
     printf("[GPU] Memory: %lu MB\n", mem_size / (1024 * 1024));
     printf("[GPU] Compute Units: %u\n", compute_units);
     printf("[GPU] FP16 Support: %s\n", ctx->device_info.supports_fp16 ? "YES" : "NO");
     
     // Create context
     ocl->context = clCreateContext(NULL, 1, &ocl->device, NULL, NULL, &err);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to create context\n");
         free(ocl);
         return -1;
     }
     
     // Create command queue
     ocl->queue = clCreateCommandQueue(ocl->context, ocl->device, 0, &err);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to create command queue\n");
         clReleaseContext(ocl->context);
         free(ocl);
         return -1;
     }
     
     ctx->native_context = ocl;
     ctx->command_queue = ocl->queue;
     ctx->initialized = true;
     
     printf("[GPU] OpenCL initialized successfully\n");
     
     return 0;
 }
 
 // Create GPU buffer
 gpu_buffer_t* gpu_buffer_create(gpu_context_t* ctx, size_t size) {
     if (!ctx || !ctx->initialized) return NULL;
     
     gpu_buffer_t* buffer = calloc(1, sizeof(gpu_buffer_t));
     if (!buffer) return NULL;
     
     opencl_context_t* ocl = (opencl_context_t*)ctx->native_context;
     
     cl_int err;
     cl_mem mem = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE, size, NULL, &err);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to create buffer (%d)\n", err);
         free(buffer);
         return NULL;
     }
     
     buffer->handle = mem;
     buffer->size_bytes = size;
     
     return buffer;
 }
 
 // Write to GPU buffer
 int gpu_buffer_write(gpu_context_t* ctx, gpu_buffer_t* buffer, const void* data, size_t size) {
     if (!ctx || !buffer || !data) return -1;
     
     opencl_context_t* ocl = (opencl_context_t*)ctx->native_context;
     
     cl_int err = clEnqueueWriteBuffer(ocl->queue, (cl_mem)buffer->handle, 
                                       CL_TRUE, 0, size, data, 0, NULL, NULL);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to write buffer (%d)\n", err);
         return -1;
     }
     
     return 0;
 }
 
 // Read from GPU buffer
 int gpu_buffer_read(gpu_context_t* ctx, gpu_buffer_t* buffer, void* data, size_t size) {
     if (!ctx || !buffer || !data) return -1;
     
     opencl_context_t* ocl = (opencl_context_t*)ctx->native_context;
     
     cl_int err = clEnqueueReadBuffer(ocl->queue, (cl_mem)buffer->handle,
                                      CL_TRUE, 0, size, data, 0, NULL, NULL);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to read buffer (%d)\n", err);
         return -1;
     }
     
     return 0;
 }
 
 // Destroy GPU buffer
 void gpu_buffer_destroy(gpu_context_t* ctx, gpu_buffer_t* buffer) {
     if (!buffer) return;
     
     if (buffer->handle) {
         clReleaseMemObject((cl_mem)buffer->handle);
     }
     
     free(buffer);
 }
 
 // Create GPU kernel
 gpu_kernel_t* gpu_kernel_create(gpu_context_t* ctx, const char* source, const char* kernel_name) {
     if (!ctx || !source || !kernel_name) return NULL;
     
     opencl_context_t* ocl = (opencl_context_t*)ctx->native_context;
     
     gpu_kernel_t* kernel = calloc(1, sizeof(gpu_kernel_t));
     if (!kernel) return NULL;
     
     cl_int err;
     
     // Create program
     cl_program program = clCreateProgramWithSource(ocl->context, 1, &source, NULL, &err);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to create program\n");
         free(kernel);
         return NULL;
     }
     
     // Build program
     err = clBuildProgram(program, 1, &ocl->device, "-cl-fast-relaxed-math", NULL, NULL);
     if (err != CL_SUCCESS) {
         // Get build log
         char log[4096];
         clGetProgramBuildInfo(program, ocl->device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
         fprintf(stderr, "[GPU] Build failed:\n%s\n", log);
         clReleaseProgram(program);
         free(kernel);
         return NULL;
     }
     
     // Create kernel
     cl_kernel cl_kern = clCreateKernel(program, kernel_name, &err);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to create kernel\n");
         clReleaseProgram(program);
         free(kernel);
         return NULL;
     }
     
     kernel->program = program;
     kernel->kernel = cl_kern;
     strncpy(kernel->name, kernel_name, sizeof(kernel->name) - 1);
     
     printf("[GPU] Created kernel: %s\n", kernel_name);
     
     return kernel;
 }
 
 // Set kernel argument
 int gpu_kernel_set_arg(gpu_kernel_t* kernel, uint32_t index, void* value, size_t size) {
     if (!kernel) return -1;
     
     cl_int err = clSetKernelArg((cl_kernel)kernel->kernel, index, size, value);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to set kernel arg %u\n", index);
         return -1;
     }
     
     return 0;
 }
 
 // Execute kernel
 int gpu_kernel_execute(gpu_context_t* ctx, gpu_kernel_t* kernel,
                        uint32_t work_dim, const size_t* global_work_size) {
     if (!ctx || !kernel) return -1;
     
     opencl_context_t* ocl = (opencl_context_t*)ctx->native_context;
     
     cl_int err = clEnqueueNDRangeKernel(ocl->queue, (cl_kernel)kernel->kernel,
                                         work_dim, NULL, global_work_size, NULL,
                                         0, NULL, NULL);
     if (err != CL_SUCCESS) {
         fprintf(stderr, "[GPU] Failed to execute kernel (%d)\n", err);
         return -1;
     }
     
     // Wait for completion
     clFinish(ocl->queue);
     
     return 0;
 }
 
 // Destroy kernel
 void gpu_kernel_destroy(gpu_kernel_t* kernel) {
     if (!kernel) return;
     
     if (kernel->kernel) {
         clReleaseKernel((cl_kernel)kernel->kernel);
     }
     if (kernel->program) {
         clReleaseProgram((cl_program)kernel->program);
     }
     
     free(kernel);
 }
 
 // Optimized matrix multiplication
 int gpu_matmul(gpu_context_t* ctx,
                gpu_buffer_t* A, gpu_buffer_t* B, gpu_buffer_t* C,
                uint32_t M, uint32_t N, uint32_t K) {
     if (!ctx || !A || !B || !C) return -1;
     
     // OpenCL kernel for matrix multiplication
     const char* matmul_kernel = 
         "__kernel void matmul(\n"
         "    __global const float* A,\n"
         "    __global const float* B,\n"
         "    __global float* C,\n"
         "    const uint M, const uint N, const uint K)\n"
         "{\n"
         "    const uint row = get_global_id(0);\n"
         "    const uint col = get_global_id(1);\n"
         "    \n"
         "    if (row < M && col < N) {\n"
         "        float sum = 0.0f;\n"
         "        for (uint k = 0; k < K; k++) {\n"
         "            sum += A[row * K + k] * B[k * N + col];\n"
         "        }\n"
         "        C[row * N + col] = sum;\n"
         "    }\n"
         "}\n";
     
     gpu_kernel_t* kernel = gpu_kernel_create(ctx, matmul_kernel, "matmul");
     if (!kernel) return -1;
     
     // Set arguments
     gpu_kernel_set_arg(kernel, 0, &A->handle, sizeof(cl_mem));
     gpu_kernel_set_arg(kernel, 1, &B->handle, sizeof(cl_mem));
     gpu_kernel_set_arg(kernel, 2, &C->handle, sizeof(cl_mem));
     gpu_kernel_set_arg(kernel, 3, &M, sizeof(uint32_t));
     gpu_kernel_set_arg(kernel, 4, &N, sizeof(uint32_t));
     gpu_kernel_set_arg(kernel, 5, &K, sizeof(uint32_t));
     
     // Execute
     size_t global_work_size[2] = {M, N};
     int result = gpu_kernel_execute(ctx, kernel, 2, global_work_size);
     
     gpu_kernel_destroy(kernel);
     
     return result;
 }
 
 // Optimized 2D convolution
 int gpu_conv2d(gpu_context_t* ctx,
                gpu_buffer_t* input, gpu_buffer_t* kernel, gpu_buffer_t* output,
                uint32_t batch, uint32_t in_channels, uint32_t out_channels,
                uint32_t height, uint32_t width, uint32_t kernel_size) {
     if (!ctx || !input || !kernel || !output) return -1;
     
     // Simplified conv2d kernel
     const char* conv2d_kernel =
         "__kernel void conv2d(\n"
         "    __global const float* input,\n"
         "    __global const float* weights,\n"
         "    __global float* output,\n"
         "    const uint height, const uint width,\n"
         "    const uint in_channels, const uint out_channels,\n"
         "    const uint kernel_size)\n"
         "{\n"
         "    const uint x = get_global_id(0);\n"
         "    const uint y = get_global_id(1);\n"
         "    const uint oc = get_global_id(2);\n"
         "    \n"
         "    if (x >= width || y >= height || oc >= out_channels) return;\n"
         "    \n"
         "    const int kh = kernel_size / 2;\n"
         "    float sum = 0.0f;\n"
         "    \n"
         "    for (uint ic = 0; ic < in_channels; ic++) {\n"
         "        for (int ky = -kh; ky <= kh; ky++) {\n"
         "            for (int kx = -kh; kx <= kh; kx++) {\n"
         "                int ix = x + kx;\n"
         "                int iy = y + ky;\n"
         "                \n"
         "                if (ix >= 0 && ix < width && iy >= 0 && iy < height) {\n"
         "                    uint input_idx = (ic * height + iy) * width + ix;\n"
         "                    uint weight_idx = ((oc * in_channels + ic) * kernel_size + (ky + kh)) * kernel_size + (kx + kh);\n"
         "                    sum += input[input_idx] * weights[weight_idx];\n"
         "                }\n"
         "            }\n"
         "        }\n"
         "    }\n"
         "    \n"
         "    uint output_idx = (oc * height + y) * width + x;\n"
         "    output[output_idx] = sum;\n"
         "}\n";
     
     gpu_kernel_t* kern = gpu_kernel_create(ctx, conv2d_kernel, "conv2d");
     if (!kern) return -1;
     
     // Set arguments
     gpu_kernel_set_arg(kern, 0, &input->handle, sizeof(cl_mem));
     gpu_kernel_set_arg(kern, 1, &kernel->handle, sizeof(cl_mem));
     gpu_kernel_set_arg(kern, 2, &output->handle, sizeof(cl_mem));
     gpu_kernel_set_arg(kern, 3, &height, sizeof(uint32_t));
     gpu_kernel_set_arg(kern, 4, &width, sizeof(uint32_t));
     gpu_kernel_set_arg(kern, 5, &in_channels, sizeof(uint32_t));
     gpu_kernel_set_arg(kern, 6, &out_channels, sizeof(uint32_t));
     gpu_kernel_set_arg(kern, 7, &kernel_size, sizeof(uint32_t));
     
     // Execute
     size_t global_work_size[3] = {width, height, out_channels};
     int result = gpu_kernel_execute(ctx, kern, 3, global_work_size);
     
     gpu_kernel_destroy(kern);
     
     return result;
 }
 
 // Cleanup
 void gpu_cleanup(gpu_context_t* ctx) {
     if (!ctx || !ctx->initialized) return;
     
     if (ctx->backend == GPU_BACKEND_OPENCL) {
         opencl_context_t* ocl = (opencl_context_t*)ctx->native_context;
         if (ocl) {
             if (ocl->queue) clReleaseCommandQueue(ocl->queue);
             if (ocl->context) clReleaseContext(ocl->context);
             free(ocl);
         }
     }
     
     ctx->initialized = false;
     
     printf("[GPU] Cleaned up\n");
 }