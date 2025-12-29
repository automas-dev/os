[org 0x7c00]
DATA          equ 0x500
STACK         equ 0x6fff
SECOND_STAGE  equ 0x7e00
KERNEL_OFFSET equ 0x8000

; 1. Save boot drive id
mov  [BOOT_DRIVE], dl

; 2. Setup stack
mov  bp,           STACK
mov  sp,           bp

; 3. Detect memory and store for stage 2 kernel
call detect_mem

; 4. Read stage 2 kernel from boot drive
call load_kernel

; 5. Setup GDT for kernel
cli
lgdt [gdt_descriptor]

; 6. Enter protected mode
mov  eax,          cr0
or   eax,          0x1   ; set 32-bit mode bit in cr0
mov  cr0,          eax
jmp  CODE_SEG:init_pm    ; far jump by using a different segment

; Stop if there are any errors
halt:
    hlt
    jmp halt

; 7. Jump to loader
[bits 32]
init_pm: ; we are now using 32-bit instructions
    mov ax, DATA_SEG ; 5. update the segment registers
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, STACK
    mov esp, ebp

    jmp SECOND_STAGE

[bits 16]
%include "lib/print.asm"
%include "lib/memory.asm"
%include "lib/disk.asm"
%include "lib/gdt.asm"

BOOT_DRIVE       db 0

times 510-($-$$) db 0
dw                  0xaa55
