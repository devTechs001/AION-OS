# AION OS - Kernel CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(aion-kernel C CXX ASM_NASM)

# Set the output name
set(TARGET_NAME aion-kernel)

# Define source directories
set(KERNEL_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)
set(KERNEL_MM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mm)
set(KERNEL_SCHED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sched)
set(KERNEL_PROCESS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/process)
set(KERNEL_FS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fs)
set(KERNEL_DRIVERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/drivers)
set(KERNEL_NET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/net)
set(KERNEL_SECURITY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/security)
set(KERNEL_AI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ai)

# Collect source files
set(KERNEL_SOURCES
    ${KERNEL_CORE_DIR}/kernel.cpp
    ${KERNEL_CORE_DIR}/init/early_init.cpp
    ${KERNEL_CORE_DIR}/init/late_init.cpp
    ${KERNEL_CORE_DIR}/init/subsystem_init.cpp
    ${KERNEL_MM_DIR}/core/memory_manager.cpp
    ${KERNEL_SCHED_DIR}/core/scheduler.cpp
    ${KERNEL_PROCESS_DIR}/core/process.cpp
    ${KERNEL_FS_DIR}/vfs/vfs.cpp
    ${KERNEL_DRIVERS_DIR}/core/driver_core.cpp
    ${KERNEL_NET_DIR}/core/net_core.cpp
    ${KERNEL_SECURITY_DIR}/crypto/core/crypto_core.cpp
    ${KERNEL_AI_DIR}/core/ai_engine.cpp
)

# Define headers
set(KERNEL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/aion/kernel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/aion/types.hpp
)

# Define arch-specific sources
set(ARCH_X86_64_DIR ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64)
set(ARCH_SOURCES
    ${ARCH_X86_64_DIR}/boot/entry.S
    ${ARCH_X86_64_DIR}/boot/setup.c
    ${ARCH_X86_64_DIR}/cpu/cpu.cpp
)

# Combine all sources
set(ALL_KERNEL_SOURCES
    ${KERNEL_SOURCES}
    ${ARCH_SOURCES}
    ${KERNEL_HEADERS}
)

# Create the executable
add_executable(${TARGET_NAME} ${ALL_KERNEL_SOURCES})

# Set properties for the executable
set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME aion
    LINKER_LANGUAGE C
)

# Include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ARCH_X86_64_DIR}/include
)

# Link libraries if any
# target_link_libraries(${TARGET_NAME})

# Set linker script
target_link_options(${TARGET_NAME} PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    -ffreestanding
    -nostdlib
    -lgcc
)

# Create a kernel binary target
add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.bin
    COMMAND objcopy -O binary $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.bin
    DEPENDS ${TARGET_NAME}
    COMMENT "Creating kernel binary"
)

add_custom_target(aion-kernel-bin ALL
    DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.bin
)