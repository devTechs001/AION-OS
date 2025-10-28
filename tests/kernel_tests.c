#include "test_framework.h"

// Memory Tests
void test_memory_allocation(void) {
    void* ptr = kmalloc(1024);
    ASSERT(ptr != NULL);
    ASSERT_EQ(((uintptr_t)ptr & 0xF), 0); // 16-byte aligned
    
    memset(ptr, 0xAA, 1024);
    for (int i = 0; i < 1024; i++) {
        ASSERT_EQ(((uint8_t*)ptr)[i], 0xAA);
    }
    
    kfree(ptr);
}

void test_memory_alignment(void) {
    void* ptr = kmalloc_aligned(4096, 4096);
    ASSERT(ptr != NULL);
    ASSERT_EQ(((uintptr_t)ptr & 0xFFF), 0); // 4KB aligned
    kfree(ptr);
}

// Process Tests
void test_process_creation(void) {
    process_t* proc = process_create("test_process", NULL);
    ASSERT(proc != NULL);
    ASSERT(proc->pid > 0);
    ASSERT_EQ(proc->state, PROCESS_READY);
    
    process_destroy(proc);
}

// File System Tests
void test_vfs_open(void) {
    int fd = vfs_open("/tmp/test.txt", O_CREAT | O_RDWR);
    ASSERT(fd >= 0);
    
    const char* data = "Hello, AION OS!";
    ssize_t written = vfs_write(fd, data, strlen(data));
    ASSERT_EQ(written, strlen(data));
    
    vfs_close(fd);
}

// AI Tests
void test_ai_memory_prediction(void) {
    process_t* proc = process_create("test", NULL);
    
    uint64_t predicted = ai_predict_memory_allocation(proc);
    ASSERT(predicted >= 4096);
    ASSERT(predicted <= 1024 * 1024 * 1024);
    
    process_destroy(proc);
}

// Network Tests
void test_tcp_connection(void) {
    int sock = socket_create(AF_INET, SOCK_STREAM, 0);
    ASSERT(sock >= 0);
    
    int result = socket_connect(sock, string_to_ip("127.0.0.1"), 8080);
    // May fail if no server listening, but socket creation should work
    
    socket_close(sock);
}

// Run all tests
void run_kernel_tests(void) {
    test_suite_t* suite = test_create_suite("Kernel Tests");
    
    test_add_test(suite, "Memory Allocation", test_memory_allocation);
    test_add_test(suite, "Memory Alignment", test_memory_alignment);
    test_add_test(suite, "Process Creation", test_process_creation);
    test_add_test(suite, "VFS Open/Write", test_vfs_open);
    test_add_test(suite, "AI Memory Prediction", test_ai_memory_prediction);
    test_add_test(suite, "TCP Socket", test_tcp_connection);
    
    test_run_suite(suite);
    test_print_results(suite);
}