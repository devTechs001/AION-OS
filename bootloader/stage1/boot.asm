; AION OS Bootloader Stage 1 - UEFI Compatible
; This is the first code that runs when the computer starts

[BITS 16]
[ORG 0x7C00]

SECTION .text
    global _start

_start:
    ; Clear interrupts during setup
    cli
    
    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; Enable A20 line for accessing >1MB memory
    call enable_a20
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Enable protected mode
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    ; Jump to 32-bit code
    jmp CODE_SEG:init_32bit

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

[BITS 32]
init_32bit:
    ; Setup 32-bit segments
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Setup stack
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Print boot message
    mov esi, boot_msg
    call print_string_32
    
    ; Load kernel from disk
    call load_kernel
    
    ; Setup paging for 64-bit mode
    call setup_paging
    
    ; Enable long mode
    call enable_long_mode
    
    ; Jump to 64-bit kernel
    jmp CODE_SEG_64:kernel_entry

print_string_32:
    pusha
    mov edx, 0xB8000  ; VGA text buffer
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0F      ; White on black
    mov [edx], ax
    add edx, 2
    jmp .loop
.done:
    popa
    ret

load_kernel:
    ; Load kernel from disk sectors
    mov ebx, KERNEL_LOAD_ADDR
    mov ecx, 100      ; Number of sectors to load
    mov edx, 1        ; Starting sector
    call disk_load
    ret

disk_load:
    ; ATA PIO disk loading
    pusha
    mov dx, 0x1F6
    mov al, 0xE0      ; Master drive
    out dx, al
    
    mov dx, 0x1F2
    mov al, cl        ; Sector count
    out dx, al
    
    mov dx, 0x1F3
    mov al, dl        ; LBA low
    out dx, al
    
    mov dx, 0x1F7
    mov al, 0x20      ; Read command
    out dx, al
    
.wait:
    in al, dx
    test al, 0x08
    jz .wait
    
    mov ecx, 256      ; Words per sector
    mov dx, 0x1F0
    rep insw
    
    popa
    ret

setup_paging:
    ; Clear page tables
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd
    
    ; Setup page tables for identity mapping
    mov edi, 0x1000
    mov dword [edi], 0x2003      ; PML4
    add edi, 0x1000
    mov dword [edi], 0x3003      ; PDPT
    add edi, 0x1000
    mov dword [edi], 0x4003      ; PDT
    add edi, 0x1000
    
    ; Map first 2MB
    mov ebx, 0x00000003
    mov ecx, 512
.set_entry:
    mov dword [edi], ebx
    add ebx, 0x1000
    add edi, 8
    loop .set_entry
    
    ret

enable_long_mode:
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ret

; Data section
boot_msg db 'AION OS Bootloader v1.0 - Loading kernel...', 0

; GDT
gdt_start:
    dq 0x0                          ; Null descriptor
CODE_SEG equ $ - gdt_start
    dw 0xFFFF                       ; Limit
    dw 0x0                          ; Base
    db 0x0                          ; Base
    db 10011010b                    ; Access
    db 11001111b                    ; Flags
    db 0x0                          ; Base
DATA_SEG equ $ - gdt_start
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
CODE_SEG_64 equ $ - gdt_start
    dq 0x00209A0000000000           ; 64-bit code segment
DATA_SEG_64 equ $ - gdt_start
    dq 0x0000920000000000           ; 64-bit data segment
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

KERNEL_LOAD_ADDR equ 0x100000
kernel_entry equ 0x100000

; Boot signature
times 510-($-$$) db 0
dw 0xAA55