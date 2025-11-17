#!/bin/bash
# AION OS Build Script

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

BUILD_DIR="$ROOT_DIR/build"
ISO_DIR="$ROOT_DIR/iso"
KERNEL_DIR="$ROOT_DIR/kernel"

echo "AION OS Build System"
echo "====================="

# Parse command line arguments
CLEAN=false
TARGET="all"

for arg in "$@"; do
    case $arg in
        clean)
            CLEAN=true
            ;;
        kernel)
            TARGET="kernel"
            ;;
        iso)
            TARGET="iso"
            ;;
        run)
            TARGET="run"
            ;;
        *)
            echo "Usage: $0 [clean|kernel|iso|run]"
            exit 1
            ;;
    esac
done

# Clean build directory if requested
if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    rm -rf "$ISO_DIR"
    mkdir -p "$BUILD_DIR/kernel" "$BUILD_DIR/userspace" "$ISO_DIR"
    echo "Clean complete."
fi

# Create necessary directories
mkdir -p "$BUILD_DIR/kernel" "$BUILD_DIR/userspace" "$ISO_DIR"

# Build kernel
if [ "$TARGET" = "all" ] || [ "$TARGET" = "kernel" ] || [ "$TARGET" = "iso" ] || [ "$TARGET" = "run" ]; then
    echo "Building kernel..."
    
    # Navigate to kernel directory and build
    cd "$KERNEL_DIR"
    
    # Use CMake to build the kernel
    if [ ! -d "build" ]; then
        mkdir -p build
    fi
    
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make -j$(nproc)
    
    cd "$ROOT_DIR"
    echo "Kernel build complete."
fi

# Create ISO if requested
if [ "$TARGET" = "iso" ] || [ "$TARGET" = "run" ]; then
    echo "Creating ISO..."
    
    # Create ISO directory structure
    mkdir -p "$ISO_DIR/boot/grub"
    
    # Copy kernel binary
    cp "$BUILD_DIR"/kernel/aion-kernel.bin "$ISO_DIR/boot/aion.bin" 2>/dev/null || echo "Kernel binary not found, skipping ISO creation"
    
    # Create GRUB config if it doesn't exist
    if [ ! -f "$ROOT_DIR/grub.cfg" ]; then
        echo "Creating default grub.cfg..."
        cat > "$ROOT_DIR/grub.cfg" << 'EOF'
menuentry "AION OS" {
    multiboot2 /boot/aion.bin
    boot
}
EOF
    fi
    
    cp "$ROOT_DIR/grub.cfg" "$ISO_DIR/boot/grub/"
    
    # Create the ISO
    grub-mkrescue -o "$ROOT_DIR/aion-os.iso" "$ISO_DIR" 2>/dev/null || echo "Warning: Could not create ISO (grub-mkrescue not found or missing dependencies)"
    
    echo "ISO creation complete."
fi

# Run in QEMU if requested
if [ "$TARGET" = "run" ]; then
    echo "Running AION OS in QEMU..."
    if command -v qemu-system-x86_64 &> /dev/null; then
        qemu-system-x86_64 -cdrom "$ROOT_DIR/aion-os.iso" -m 2G -smp 4 -vga std -serial stdio
    else
        echo "Error: qemu-system-x86_64 not found. Please install QEMU to run the OS."
    fi
fi

echo "Build process completed."