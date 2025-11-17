# AION OS - Development Environment Setup

This document outlines the setup and configuration for developing AION OS.

## Prerequisites

### System Requirements
- x86-64 Linux system (Ubuntu 20.04+, Fedora 35+, Arch)
- Minimum 8GB RAM (16GB recommended)
- At least 50GB free disk space
- Internet connection for dependencies

### Required Packages (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y \
    build-essential \
    nasm \
    gcc \
    g++ \
    xorriso \
    grub-pc-bin \
    grub-efi-amd64-bin \
    mtools \
    qemu-system-x86 \
    gdb \
    git \
    cmake \
    python3 \
    python3-pip \
    clang \
    lld \
    pkg-config
```

### Required Packages (Fedora)
```bash
sudo dnf install -y \
    gcc gcc-c++ \
    nasm \
    xorriso \
    grub2-tools \
    grub2-efi-x64 \
    mtools \
    qemu-kvm \
    gdb \
    git \
    cmake \
    python3 \
    python3-pip \
    clang \
    lld \
    pkgconfig
```

## Toolchain Setup

### Cross-Compiler (Optional)
AION OS can be built with a cross-compiler to avoid depending on host system tools:

```bash
# Build cross-compiler (this takes time)
make build-cross-compiler
```

### AI Framework Dependencies
For AI features, install:
```bash
pip3 install tensorflowlite tensorflow numpy pillow
```

## Build Configuration

### Default Configuration
The default build configuration is optimized for development:

```
config/build/build.config:
{
  "debug": true,
  "optimization": "O2",
  "target_arch": "x86_64",
  "features": {
    "ai": true,
    "networking": true,
    "graphics": false,
    "security": true
  }
}
```

### Feature Configuration
You can customize features in `config/features.config`:

```
# Enable AI features
CONFIG_AI_INTEGRATION=y
CONFIG_TENSORFLOW_LITE=y
CONFIG_ONNX_RUNTIME=n

# Enable networking
CONFIG_NETWORKING=y
CONFIG_TCP=y
CONFIG_UDP=y

# Enable security features
CONFIG_SECURITY=y
CONFIG_CRYPTO=y
CONFIG_MANDATORY_ACCESS_CONTROL=n

# Enable graphics
CONFIG_GRAPHICS=y
CONFIG_FRAMEBUFFER=y
CONFIG_WAYLAND=n
```

## Build Targets

### Primary Targets
- `make all` - Build complete system
- `make kernel` - Build kernel only
- `make userspace` - Build userspace only
- `make iso` - Create bootable ISO
- `make run` - Build and run in QEMU
- `make debug` - Build with debug symbols
- `make test` - Run test suite
- `make clean` - Clean build artifacts

### Development Targets
- `make qemu` - Run in QEMU
- `make gdb` - Start GDB debugging
- `make stats` - Show build statistics
- `make docs` - Generate documentation

## Development Workflow

### Quick Build and Test
```bash
make clean && make iso
make run
```

### Debugging Build
```bash
make debug
qemu-system-x86_64 -s -S -cdrom aion-os.iso
# In another terminal: gdb ./build/kernel/bin
```

### Code Style
- Follow Linux kernel coding style for C code
- Use C++20 features where appropriate
- All code must pass static analysis
- Documentation required for public APIs
- Tests required for all functionality

## Directory Structure for Development

### Development Directories
- `kernel/` - Kernel source code
- `userland/` - Userspace applications
- `docs/` - Documentation
- `tests/` - Test suite
- `tools/` - Development tools
- `build/` - Build artifacts
- `iso/` - ISO build directory

### Key Files
- `CMakeLists.txt` - Main CMake configuration
- `Makefile` - Main Makefile
- `linker.ld` - Kernel linker script
- `grub.cfg` - Bootloader configuration
- `config/` - Configuration files

## Testing Environment

### Unit Testing
```bash
make test-unit
```

### Integration Testing
```bash
make test-integration
```

### Performance Testing
```bash
make test-performance
```

## Common Issues and Solutions

### Missing Dependencies
```bash
# Check for missing dependencies
make check-deps
```

### Build Errors
- Clean build: `make clean && make all`
- Check compiler version: `gcc --version`
- Ensure proper package installation

### QEMU Issues
- Check QEMU installation: `qemu-system-x86_64 --version`
- Ensure KVM support: `ls /dev/kvm`
- Try without KVM: `qemu-system-x86_64 -no-kvm -cdrom aion-os.iso`

## Advanced Configuration

### Custom Kernel Config
```bash
# Generate custom config
make menuconfig
# Or manually edit config/kernel/kernel.config
```

### Cross Architecture
```bash
# Build for ARM64 (when support is added)
make ARCH=arm64
```

This setup enables full development capability for AION OS with all the features mentioned in the roadmap and implementation plans.