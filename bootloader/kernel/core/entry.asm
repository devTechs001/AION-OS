; AION OS Kernel Entry Point
[BITS 64]

section .text
    global _start
    extern kernel_main

_start:
    ; Setup stack
    mov rsp, stack_top
    
    ; Clear RFLAGS
    push 0
    popfq
    
    ; Call kernel main
    call kernel_main
    
    ; Halt if kernel returns
    cli
.halt:
    hlt
    jmp .halt

section .bss
    align 16
stack_bottom:
    resb 16384  ; 16KB stack
stack_top: