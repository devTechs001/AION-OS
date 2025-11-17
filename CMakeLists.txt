# AION OS - Main CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(AION-OS C CXX ASM_NASM)

# Set C and C++ standards
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -fno-stack-protector -fno-exceptions -fno-rtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -fno-stack-protector -fno-exceptions -fno-rtti -nostdlib")
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add subdirectories
add_subdirectory(kernel)
add_subdirectory(lib)
add_subdirectory(userspace)

# Create ISO target
add_custom_target(iso
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/iso/boot/grub
    COMMAND cp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/aion.bin ${CMAKE_BINARY_DIR}/iso/boot/
    COMMAND cp /home/darkhat/general-projects/AION-OS/AION-OS-new/grub.cfg ${CMAKE_BINARY_DIR}/iso/boot/grub/
    COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/aion-os.iso ${CMAKE_BINARY_DIR}/iso
    DEPENDS aion-kernel
    COMMENT "Creating ISO image"
)