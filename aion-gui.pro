TEMPLATE = subdirs

SUBDIRS += bootloader/kernel/gui \
           userland/compositor \
           userland/ai-ide

# Add GUI-specific configurations
QT += core widgets gui

# Include paths for AION-OS
INCLUDEPATH += \
    $$PWD/bootloader/kernel \
    $$PWD/bootloader/kernel/gui \
    $$PWD/bootloader/kernel/graphics \
    $$PWD/bootloader/kernel/core

# Define target OS as AION-OS
DEFINES += AION_OS

# Compiler flags specific for kernel development
QMAKE_CFLAGS += -ffreestanding -fno-stack-protector -fno-stack-check
QMAKE_CXXFLAGS += -ffreestanding -fno-stack-protector -fno-stack-check

# Linker flags
QMAKE_LFLAGS += -T $$PWD/linker.Id -nostdlib

# Project name
TARGET = aion-gui
