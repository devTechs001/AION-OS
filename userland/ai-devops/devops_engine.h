#ifndef DEVOPS_ENGINE_H
#define DEVOPS_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// Build Status
typedef enum {
    BUILD_QUEUED,
    BUILD_IN_PROGRESS,
    BUILD_SUCCESS,
    BUILD_FAILED,
    BUILD_CANCELLED
} build_status_t;

// Test Result
typedef struct {
    char test_name[128];
    bool passed;
    uint64_t execution_time_us;
    char error_message[512];
    char stack_trace[2048];
} test_result_t;

// Build Configuration
typedef struct {
    char project_path[256];
    char build_command[512];
    char test_command[512];
    
    // Build options
    bool enable_optimizations;
    bool enable_debug_symbols;
    bool enable_warnings_as_errors;
    
    // Parallel build
    int num_build_threads;
    
    // AI optimization
    bool ai_optimize_build_order;
    bool ai_predict_failures;
    bool ai_cache_results;
} build_config_t;

// Build Job
typedef struct {
    int job_id;
    build_status_t status;
    
    char commit_hash[64];
    char branch[128];
    char author[128];
    
    uint64_t start_time;
    uint64_t end_time;
    uint64_t duration_ms;
    
    // Results
    int tests_run;
    int tests_passed;
    int tests_failed;
    test_result_t* test_results;
    
    // Build output
    char* build_log;
    size_t log_size;
    
    // AI predictions
    struct {
        float predicted_success_probability;
        float predicted_build_time_ms;
        char* predicted_failure_points[16];
        int num_predictions;
    } ai_analysis;
    
    // Artifacts
    char artifact_path[256];
    uint64_t artifact_size;
} build_job_t;

// CI/CD Pipeline
typedef struct {
    char name[128];
    
    // Stages
    struct {
        char name[64];
        char command[512];
        bool allow_failure;
        int timeout_seconds;
    } stages[16];
    int num_stages;
    
    // Triggers
    bool on_push;
    bool on_pull_request;
    bool on_schedule;
    
    // Statistics
    uint64_t total_runs;
    uint64_t successful_runs;
    uint64_t failed_runs;
    float success_rate;
} ci_pipeline_t;

// Deployment Configuration
typedef struct {
    char environment[64];  // dev, staging, production
    char target_host[256];
    int target_port;
    
    // Deployment strategy
    enum {
        DEPLOY_ROLLING,
        DEPLOY_BLUE_GREEN,
        DEPLOY_CANARY
    } strategy;
    
    // Health checks
    char health_check_url[256];
    int health_check_interval_seconds;
    int health_check_retries;
    
    // Rollback
    bool auto_rollback_on_failure;
    char previous_version[64];
} deployment_config_t;

// Performance Metrics
typedef struct {
    // Build metrics
    uint32_t avg_build_time_ms;
    uint32_t avg_test_time_ms;
    
    // Test coverage
    float code_coverage_percent;
    int total_lines;
    int covered_lines;
    
    // Quality metrics
    int bugs_found;
    int security_issues;
    int code_smells;
    
    // AI predictions
    float predicted_failure_rate;
    uint32_t predicted_next_build_time_ms;
} devops_metrics_t;

// DevOps Engine
typedef struct {
    // Active jobs
    build_job_t* jobs[256];
    int num_jobs;
    
    // Pipelines
    ci_pipeline_t* pipelines[64];
    int num_pipelines;
    
    // Metrics
    devops_metrics_t metrics;
    
    // AI models
    void* build_time_predictor;
    void* failure_predictor;
    void* test_optimizer;
    
    spinlock_t lock;
} devops_engine_t;

// Function Prototypes
void devops_init(void);
devops_engine_t* devops_get_engine(void);

// Build Management
build_job_t* devops_create_build_job(build_config_t* config);
void devops_start_build(build_job_t* job);
void devops_cancel_build(build_job_t* job);
build_status_t devops_get_build_status(int job_id);

// Testing
test_result_t** devops_run_tests(const char* project_path, int* num_tests);
void devops_generate_test_report(build_job_t* job, const char* output_file);
float devops_calculate_code_coverage(const char* project_path);

// CI/CD Pipeline
ci_pipeline_t* devops_create_pipeline(const char* name);
void devops_add_pipeline_stage(ci_pipeline_t* pipeline, const char* name, const char* command);
void devops_trigger_pipeline(ci_pipeline_t* pipeline, const char* commit_hash);

// Deployment
int devops_deploy(deployment_config_t* config, const char* artifact_path);
bool devops_health_check(deployment_config_t* config);
int devops_rollback(deployment_config_t* config);

// AI Features
float devops_ai_predict_build_time(build_config_t* config);
float devops_ai_predict_failure_probability(build_job_t* job);
char** devops_ai_suggest_optimizations(const char* project_path, int* num_suggestions);
void devops_ai_optimize_test_order(test_result_t** tests, int num_tests);

// Monitoring
void devops_start_monitoring(void);
devops_metrics_t* devops_get_metrics(void);
void devops_alert(const char* message, int severity);

// Reporting
void devops_generate_report(const char* output_file);

#endif // DEVOPS_ENGINE_H