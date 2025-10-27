# AION OS Build System
# Comprehensive Makefile for building the entire OS

# Compiler and tools
CC = x86_64-elf-gcc
AS = x86_64-elf-as
LD = x86_64-elf-ld
NASM = nasm
QEMU = qemu-system-x86_64

# Flags
CFLAGS = -Wall -Wextra -O2 -std=gnu11 -ffreestanding -fno-stack-protector \
         -fno-stack-check -fno-strict-aliasing -fno-common \
         -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
         -mcmodel=kernel -march=x86-64 -m64

LDFLAGS = -T linker.ld -nostdlib -z max-page-size=0x1000

NASMFLAGS = -f elf64

# Directories
KERNEL_DIR = kernel
BOOT_DIR = bootloader
BUILD_DIR = build
ISO_DIR = iso

# Source files
KERNEL_C_SOURCES = $(shell find $(KERNEL_DIR) -name '*.c')
KERNEL_ASM_SOURCES = $(shell find $(KERNEL_DIR) -name '*.asm')
BOOT_ASM_SOURCES = $(shell find $(BOOT_DIR) -name '*.asm')

# Object files
KERNEL_C_OBJECTS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(KERNEL_C_SOURCES))
KERNEL_ASM_OBJECTS = $(patsubst %.asm, $(BUILD_DIR)/%.o, $(KERNEL_ASM_SOURCES))
BOOT_OBJECTS = $(patsubst %.asm, $(BUILD_DIR)/%.o, $(BOOT_ASM_SOURCES))

# Targets
KERNEL = $(BUILD_DIR)/kernel.elf
BOOTLOADER = $(BUILD_DIR)/boot.bin
ISO = aion-os.iso

# Colors for output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
BLUE = \033[0;34m
NC = \033[0m # No Color

.PHONY: all clean run debug iso kernel bootloader

all: print-banner $(ISO)
	@echo "$(GREEN)[BUILD] AION OS build complete!$(NC)"
	@echo "$(BLUE)[INFO] ISO image: $(ISO)$(NC)"
	@echo "$(YELLOW)[TIP] Run 'make qemu' to test in QEMU$(NC)"

print-banner:
	@echo "$(BLUE)================================================$(NC)"
	@echo "$(BLUE)     Building AION OS - AI-Powered OS v1.0     $(NC)"
	@echo "$(BLUE)================================================$(NC)"

# Build kernel
kernel: $(KERNEL)
	@echo "$(GREEN)[KERNEL] Kernel built successfully$(NC)"

$(KERNEL): $(KERNEL_C_OBJECTS) $(KERNEL_ASM_OBJECTS)
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)[LD] Linking kernel...$(NC)"
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "$(GREEN)[LD] Kernel linked: $@$(NC)"

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "$(BLUE)[CC] Compiling $<$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble ASM files
$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	@echo "$(BLUE)[AS] Assembling $<$(NC)"
	$(NASM) $(NASMFLAGS) $< -o $@

# Build bootloader
bootloader: $(BOOTLOADER)
	@echo "$(GREEN)[BOOT] Bootloader built successfully$(NC)"

$(BOOTLOADER): $(BOOT_DIR)/stage1/boot.asm
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)[NASM] Building bootloader...$(NC)"
	$(NASM) -f bin $< -o $@

# Create ISO image
iso: $(ISO)

$(ISO): $(KERNEL) $(BOOTLOADER) grub.cfg
	@echo "$(YELLOW)[ISO] Creating ISO image...$(NC)"
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL) $(ISO_DIR)/boot/
	@cp grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null
	@echo "$(GREEN)[ISO] ISO created: $@$(NC)"

# Run in QEMU
qemu: $(ISO)
	@echo "$(BLUE)[QEMU] Starting AION OS in QEMU...$(NC)"
	$(QEMU) -cdrom $(ISO) -m 4G -smp 4 \
	        -enable-kvm -cpu host \
	        -vga std -serial stdio \
	        -drive file=disk.img,if=ide

# Run with debugging
debug: $(ISO)
	@echo "$(YELLOW)[DEBUG] Starting AION OS in debug mode...$(NC)"
	$(QEMU) -cdrom $(ISO) -m 4G -smp 4 \
	        -s -S \
	        -serial stdio \
	        -monitor telnet:127.0.0.1:1234,server,nowait &
	@echo "$(GREEN)[DEBUG] GDB can now connect to localhost:1234$(NC)"
	@gdb $(KERNEL) -ex "target remote localhost:1234"

# Create disk image
disk.img:
	@echo "$(BLUE)[DISK] Creating disk image...$(NC)"
	@dd if=/dev/zero of=disk.img bs=1M count=512
	@echo "$(GREEN)[DISK] Disk image created: disk.img$(NC)"

# Clean build artifacts
clean:
	@echo "$(RED)[CLEAN] Removing build artifacts...$(NC)"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ISO_DIR)
	@rm -f $(ISO)
	@rm -f disk.img
	@echo "$(GREEN)[CLEAN] Clean complete$(NC)"

# Generate documentation
docs:
	@echo "$(BLUE)[DOCS] Generating documentation...$(NC)"
	@doxygen Doxyfile
	@echo "$(GREEN)[DOCS] Documentation generated in docs/html$(NC)"

# Run tests
test: $(KERNEL)
	@echo "$(BLUE)[TEST] Running kernel tests...$(NC)"
	@$(BUILD_DIR)/run_tests.sh
	@echo "$(GREEN)[TEST] All tests passed!$(NC)"

# Install development dependencies
deps:
	@echo "$(YELLOW)[DEPS] Installing dependencies...$(NC)"
	@sudo apt-get update
	@sudo apt-get install -y build-essential nasm qemu-system-x86 \
	                         grub-pc-bin grub-common xorriso \
	                         gcc-multilib g++-multilib
	@echo "$(GREEN)[DEPS] Dependencies installed$(NC)"

# Build statistics
stats:
	@echo "$(BLUE)================================================$(NC)"
	@echo "$(BLUE)           AION OS Build Statistics            $(NC)"
	@echo "$(BLUE)================================================$(NC)"
	@echo "C files:     $$(find $(KERNEL_DIR) -name '*.c' | wc -l)"
	@echo "ASM files:   $$(find . -name '*.asm' | wc -l)"
	@echo "Headers:     $$(find . -name '*.h' | wc -l)"
	@echo "Total LOC:   $$(find . -name '*.[ch]' -o -name '*.asm' | xargs wc -l | tail -1)"
	@echo "Kernel size: $$(du -h $(KERNEL) 2>/dev/null | cut -f1)"
	@echo "ISO size:    $$(du -h $(ISO) 2>/dev/null | cut -f1)"
	@echo "$(BLUE)================================================$(NC)"

# Help
help:
	@echo "$(BLUE)AION OS Build System Commands:$(NC)"
	@echo "  make all     - Build complete OS"
	@echo "  make kernel  - Build kernel only"
	@echo "  make iso     - Create bootable ISO"
	@echo "  make qemu    - Run OS in QEMU"
	@echo "  make debug   - Run with GDB debugging"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make docs    - Generate documentation"
	@echo "  make test    - Run tests"
	@echo "  make deps    - Install dependencies"
	@echo "  make stats   - Show build statistics"