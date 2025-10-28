// AION OS Virtual File System with AI Optimization
#include "vfs.h"
#include "../memory/memory.h"
#include "../ai/predictor.h"

// VFS structures
static vfs_node_t *vfs_root = NULL;
static filesystem_t registered_filesystems[MAX_FILESYSTEMS];
static uint32_t num_filesystems = 0;

// Mount points
static mount_point_t mount_points[MAX_MOUNT_POINTS];
static uint32_t num_mounts = 0;

// File descriptor table
static file_descriptor_t fd_table[MAX_FILE_DESCRIPTORS];

// AI file system optimizer
static ai_fs_optimizer_t *fs_optimizer;

// VFS cache
static vfs_cache_t *vfs_cache;

// Initialize VFS
void vfs_init(void) {
    kprintf("[VFS] Initializing virtual file system...\n");
    
    // Clear structures
    memset(registered_filesystems, 0, sizeof(registered_filesystems));
    memset(mount_points, 0, sizeof(mount_points));
    memset(fd_table, 0, sizeof(fd_table));
    
    // Initialize AI optimizer
    fs_optimizer = ai_fs_optimizer_create();
    
    // Initialize cache with AI-predicted size
    uint32_t cache_size = fs_optimizer->predict_cache_size();
    vfs_cache = vfs_cache_create(cache_size);
    
    // Create root node
    vfs_root = vfs_create_node("/", VFS_DIRECTORY, 0755);
    vfs_root->mount_point = NULL;
    
    // Register built-in filesystems
    register_filesystem("tmpfs", &tmpfs_ops);
    register_filesystem("procfs", &procfs_ops);
    register_filesystem("devfs", &devfs_ops);
    register_filesystem("sysfs", &sysfs_ops);
    
    // Mount root filesystem
    mount("tmpfs", "/", "tmpfs", 0, NULL);
    
    // Create standard directories
    vfs_mkdir("/dev", 0755);
    vfs_mkdir("/proc", 0555);
    vfs_mkdir("/sys", 0555);
    vfs_mkdir("/tmp", 1777);
    vfs_mkdir("/home", 0755);
    vfs_mkdir("/bin", 0755);
    vfs_mkdir("/etc", 0755);
    
    // Mount special filesystems
    mount("devfs", "/dev", "devfs", 0, NULL);
    mount("procfs", "/proc", "procfs", MS_RDONLY, NULL);
    mount("sysfs", "/sys", "sysfs", MS_RDONLY, NULL);
    
    kprintf("[VFS] Virtual file system initialized\n");
}

// Register filesystem
int register_filesystem(const char *name, filesystem_ops_t *ops) {
    if (num_filesystems >= MAX_FILESYSTEMS) {
        return -ENOMEM;
    }
    
    filesystem_t *fs = &registered_filesystems[num_filesystems++];
    strncpy(fs->name, name, FS_NAME_MAX);
    fs->ops = ops;
    
    kprintf("[VFS] Registered filesystem: %s\n", name);
    return 0;
}

// Mount filesystem
int mount(const char *source, const char *target, 
          const char *fstype, unsigned long flags, void *data) {
    // Find filesystem type
    filesystem_t *fs = NULL;
    for (uint32_t i = 0; i < num_filesystems; i++) {
        if (strcmp(registered_filesystems[i].name, fstype) == 0) {
            fs = &registered_filesystems[i];
            break;
        }
    }
    
    if (!fs) {
        kprintf("[VFS] Unknown filesystem: %s\n", fstype);
        return -ENODEV;
    }
    
    // Find or create mount point
    vfs_node_t *mount_node = vfs_lookup_path(target);
    if (!mount_node) {
        mount_node = vfs_mkdir(target, 0755);
    }
    
    if (!mount_node) {
        return -ENOENT;
    }
    
    // Create mount point entry
    mount_point_t *mp = &mount_points[num_mounts++];
    strncpy(mp->source, source, PATH_MAX);
    strncpy(mp->target, target, PATH_MAX);
    mp->filesystem = fs;
    mp->flags = flags;
    mp->data = data;
    
    // Mount filesystem
    if (fs->ops->mount) {
        int result = fs->ops->mount(mp);
        if (result < 0) {
            num_mounts--;
            return result;
        }
    }
    
    mount_node->mount_point = mp;
    
    kprintf("[VFS] Mounted %s on %s (type %s)\n", source, target, fstype);
    return 0;
}

// Open file
int vfs_open(const char *path, int flags, mode_t mode) {
    // AI prediction: Pre-cache likely files
    fs_optimizer->predict_next_open(path);
    
    // Check cache first
    vfs_node_t *node = vfs_cache_lookup(vfs_cache, path);
    
    if (!node) {
        // Lookup path
        node = vfs_lookup_path(path);
        
        if (!node) {
            if (flags & O_CREAT) {
                // Create file
                node = vfs_create_file(path, mode);
                if (!node) {
                    return -ENOENT;
                }
            } else {
                return -ENOENT;
            }
        }
        
        // Add to cache
        vfs_cache_add(vfs_cache, path, node);
    }
    
    // Find free file descriptor
    int fd = -1;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (!fd_table[i].in_use) {
            fd = i;
            break;
        }
    }
    
    if (fd < 0) {
        return -EMFILE;
    }
    
    // Initialize file descriptor
    file_descriptor_t *file = &fd_table[fd];
    file->in_use = true;
    file->node = node;
    file->flags = flags;
    file->position = 0;
    file->refcount = 1;
    
    // Call filesystem-specific open
    if (node->ops && node->ops->open) {
        int result = node->ops->open(node, file);
        if (result < 0) {
            file->in_use = false;
            return result;
        }
    }
    
    // AI learning: Record file access pattern
    fs_optimizer->record_file_access(path, flags);
    
    return fd;
}

// Read from file
ssize_t vfs_read(int fd, void *buffer, size_t count) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].in_use) {
        return -EBADF;
    }
    
    file_descriptor_t *file = &fd_table[fd];
    
    if (!(file->flags & (O_RDONLY | O_RDWR))) {
        return -EBADF;
    }
    
    // AI-optimized read strategy
    read_strategy_t strategy = fs_optimizer->get_read_strategy(file, count);
    
    ssize_t result = 0;
    
    switch (strategy) {
        case READ_DIRECT:
            // Direct read without caching
            if (file->node->ops && file->node->ops->read) {
                result = file->node->ops->read(file->node, buffer, 
                                              count, file->position);
            }
            break;
            
        case READ_CACHED:
            // Read through cache
            result = vfs_cache_read(vfs_cache, file, buffer, count);
            break;
            
        case READ_PREFETCH:
            // Prefetch and read
            vfs_prefetch(file, count);
            result = vfs_cache_read(vfs_cache, file, buffer, count);
            break;
    }
    
    if (result > 0) {
        file->position += result;
    }
    
    return result;
}

// Write to file
ssize_t vfs_write(int fd, const void *buffer, size_t count) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].in_use) {
        return -EBADF;
    }
    
    file_descriptor_t *file = &fd_table[fd];
    
    if (!(file->flags & (O_WRONLY | O_RDWR))) {
        return -EBADF;
    }
    
    ssize_t result = 0;
    
    if (file->node->ops && file->node->ops->write) {
        result = file->node->ops->write(file->node, buffer, 
                                       count, file->position);
    }
    
    if (result > 0) {
        file->position += result;
        file->node->size = max(file->node->size, file->position);
        file->node->mtime = get_system_time();
    }
    
    // AI: Predict when to flush
    if (fs_optimizer->should_flush(file)) {
        vfs_sync(fd);
    }
    
    return result;
}

// Lookup path in VFS
vfs_node_t* vfs_lookup_path(const char *path) {
    if (!path || path[0] != '/') {
        return NULL;
    }
    
    vfs_node_t *current = vfs_root;
    char *path_copy = strdup(path);
    char *token = strtok(path_copy, "/");
    
    while (token != NULL) {
        // Check if current node is a mount point
        if (current->mount_point) {
            // Follow mount point
            current = current->mount_point->root;
        }
        
        // Look up child
        vfs_node_t *child = NULL;
        if (current->ops && current->ops->lookup) {
            child = current->ops->lookup(current, token);
        }
        
        if (!child) {
            free(path_copy);
            return NULL;
        }
        
        current = child;
        token = strtok(NULL, "/");
    }
    
    free(path_copy);
    return current;
}

// Create directory
int vfs_mkdir(const char *path, mode_t mode) {
    // Parse parent path and directory name
    char *parent_path = strdup(path);
    char *dir_name = strrchr(parent_path, '/');
    
    if (dir_name) {
        *dir_name = '\0';
        dir_name++;
    } else {
        free(parent_path);
        return -EINVAL;
    }
    
    // Find parent directory
    vfs_node_t *parent = vfs_lookup_path(parent_path[0] ? parent_path : "/");
    free(parent_path);
    
    if (!parent) {
        return -ENOENT;
    }
    
    // Create directory
    if (parent->ops && parent->ops->mkdir) {
        return parent->ops->mkdir(parent, dir_name, mode);
    }
    
    return -ENOSYS;
}

// AI-powered file prefetching
void vfs_prefetch(file_descriptor_t *file, size_t count) {
    // Predict next access pattern
    prefetch_prediction_t pred = fs_optimizer->predict_prefetch(file, count);
    
    if (pred.should_prefetch) {
        // Prefetch predicted data
        size_t prefetch_size = pred.prefetch_size;
        off_t prefetch_offset = file->position + count;
        
        void *prefetch_buffer = kmalloc(prefetch_size);
        if (prefetch_buffer) {
            if (file->node->ops && file->node->ops->read) {
                file->node->ops->read(file->node, prefetch_buffer,
                                     prefetch_size, prefetch_offset);
            }
            
            // Add to cache
            vfs_cache_add_data(vfs_cache, file->node, prefetch_offset,
                              prefetch_buffer, prefetch_size);
        }
    }
}