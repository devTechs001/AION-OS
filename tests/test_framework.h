#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>

// Test Result
typedef struct {
    char name[128];
    bool passed;
    uint64_t execution_time_us;
    char error_message[256];
} test_result_t;

// Test Suite
typedef struct {
    char name[64];
    test_result_t* tests;
    int num_tests;
    int tests_passed;
    int tests_failed;
} test_suite_t;

// Assertions
#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            test_fail(__FILE__, __LINE__, #cond); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            test_fail_eq(__FILE__, __LINE__, #a, #b, (uint64_t)(a), (uint64_t)(b)); \
            return; \
        } \
    } while(0)

#define ASSERT_NEQ(a, b) \
    do { \
        if ((a) == (b)) { \
            test_fail(__FILE__, __LINE__, #a " == " #b); \
            return; \
        } \
    } while(0)

// Test Functions
void test_init(void);
test_suite_t* test_create_suite(const char* name);
void test_add_test(test_suite_t* suite, const char* name, void (*test_func)(void));
void test_run_suite(test_suite_t* suite);
void test_print_results(test_suite_t* suite);

// Test Utilities
void test_fail(const char* file, int line, const char* message);
void test_fail_eq(const char* file, int line, const char* expr_a, const char* expr_b,
                  uint64_t val_a, uint64_t val_b);

#endif // TEST_FRAMEWORK_H