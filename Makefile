# AION OS Makefile
# Top-level build system for AION OS

# Default target
.DEFAULT_GOAL := all

# Directories
BUILD_DIR := build
KERNEL_DIR := kernel
USERLAND_DIR := userspace
ISO_DIR := iso

# Tools
CMAKE := cmake
MAKE := make
GRUBMKRESCUE := grub-mkrescue
QEMU := qemu-system-x86_64

# Build kernel only
.PHONY: kernel
kernel:
	@echo "Building AION OS kernel..."
	@mkdir -p $(BUILD_DIR)/kernel
	@cd $(BUILD_DIR)/kernel && $(CMAKE) ../..
	@$(MAKE) -C $(BUILD_DIR)/kernel

# Build userspace (placeholder - will be expanded)
.PHONY: userspace
userspace:
	@echo "Building AION OS userspace components..."
	@mkdir -p $(BUILD_DIR)/userspace
	@cd $(BUILD_DIR)/userspace && $(CMAKE) ../../$(USERLAND_DIR)
	@$(MAKE) -C $(BUILD_DIR)/userspace

# Build all components
.PHONY: all
all: kernel userspace

# Create ISO image
.PHONY: iso
iso: kernel
	@echo "Creating ISO image..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/kernel/aion.bin $(ISO_DIR)/boot/ 2>/dev/null || echo "Warning: Kernel binary not found"
	@cp grub.cfg $(ISO_DIR)/boot/grub/ 2>/dev/null || echo "Warning: grub.cfg not found"
	@$(GRUBMKRESCUE) -o aion-os.iso $(ISO_DIR) 2>/dev/null || echo "Warning: Could not create ISO (grub-mkrescue may not be installed)"

# Run in QEMU
.PHONY: run
run: iso
	@echo "Running AION OS in QEMU..."
	@$(QEMU) -cdrom aion-os.iso -m 2G -smp 4 -vga std -serial stdio

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(ISO_DIR) aion-os.iso

# Full clean - remove all generated files
.PHONY: distclean
distclean: clean
	@rm -f tags cscope.out cscope.in.out cscope.po.out

# Generate tags for code navigation
.PHONY: tags
tags:
	@echo "Generating tags..."
	@find . -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | grep -v "$(BUILD_DIR)" | xargs ctags

# Generate documentation (placeholder)
.PHONY: docs
docs:
	@echo "Generating documentation..."
	@mkdir -p docs/_build
	@echo "Documentation generation would occur here"

# Run tests (placeholder)
.PHONY: test
test:
	@echo "Running tests..."
	@echo "Test framework would run here"

# Install to disk (placeholder)
.PHONY: install
install:
	@echo "Installing AION OS..."
	@echo "Installation would occur here"

# Help target
.PHONY: help
help:
	@echo "AION OS - Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build all components (default)"
	@echo "  kernel     - Build kernel only"
	@echo "  userspace  - Build userspace only"
	@echo "  iso        - Create ISO image"
	@echo "  run        - Run OS in QEMU"
	@echo "  clean      - Clean build artifacts"
	@echo "  distclean  - Full clean"
	@echo "  tags       - Generate code tags"
	@echo "  docs       - Generate documentation"
	@echo "  test       - Run tests"
	@echo "  install    - Install OS"
	@echo "  help       - Show this help"