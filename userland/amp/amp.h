#ifndef APM_H
#define APM_H

#include <stdint.h>
#include <stdbool.h>

// Package Status
#define PKG_STATUS_NOT_INSTALLED    0
#define PKG_STATUS_INSTALLED        1
#define PKG_STATUS_OUTDATED         2
#define PKG_STATUS_BROKEN           3

// Package Structure
typedef struct {
    char name[128];
    char version[32];
    char description[512];
    char author[128];
    char license[64];
    
    uint64_t size;
    uint64_t installed_size;
    
    char* dependencies[32];
    int num_dependencies;
    
    char* files[1024];
    int num_files;
    
    uint8_t checksum[32];
    
    int status;
    uint64_t install_date;
} package_t;

// Package Database
typedef struct {
    package_t packages[4096];
    int num_packages;
    
    char repository_url[256];
    uint64_t last_update;
    
    spinlock_t lock;
} package_database_t;

// Package Manager Operations
typedef struct {
    // Core Operations
    int (*install)(const char* package_name);
    int (*remove)(const char* package_name);
    int (*update)(const char* package_name);
    int (*upgrade_all)(void);
    
    // Query Operations
    package_t* (*search)(const char* query);
    package_t* (*info)(const char* package_name);
    package_t** (*list_installed)(void);
    
    // Repository Operations
    int (*update_database)(void);
    int (*add_repository)(const char* url);
    int (*remove_repository)(const char* url);
    
    // AI-Enhanced Features
    char** (*ai_recommend)(const char* package_name);
    int (*ai_auto_cleanup)(void);
    bool (*ai_check_compatibility)(const char* package_name);
} package_manager_t;

// Function Prototypes
void apm_init(void);
package_manager_t* apm_get_manager(void);

// Package Operations
int apm_install(const char* package_name);
int apm_remove(const char* package_name);
int apm_update(const char* package_name);
int apm_search(const char* query);

// AI Features
void apm_ai_analyze_dependencies(package_t* pkg);
void apm_ai_suggest_packages(const char* category);
void apm_ai_optimize_installation_order(package_t** packages, int count);

#endif // APM_H