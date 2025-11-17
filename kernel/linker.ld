/* AION OS Linker Script */
ENTRY(long_mode_start)

SECTIONS
{
    /* Kernel will be loaded at 0x100000 (1MB) */
    . = 0x100000;

    kernel_start = .;

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    kernel_end = .;
}