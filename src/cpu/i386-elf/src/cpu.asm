[bits 32]

gdtr dw 0 ; For limit storage
     dd 0 ; For base storage

global load_gdt
load_gdt:
    mov ax, [esp + 4]
    mov [gdtr], ax
    mov eax, [esp + 8]
    mov [gdtr + 2], eax
    lgdt [gdtr]
    call reloadSegments
    ret

reloadSegments:
   ; Reload CS register containing code selector:
   jmp   0x08:.reload_CS ; 0x08 is a stand-in for your code segment
.reload_CS:
   ; Reload data segment registers:
   mov   ax, 0x10 ; 0x10 is a stand-in for your data segment
   mov   ds, ax
   mov   es, ax
   mov   fs, ax
   mov   gs, ax
   mov   ss, ax
   ret

global flush_tss
flush_tss:
    mov ax, (5 * 8) | 0 ; fifth 8-byte selector, symbolically OR-ed with 0 to set the RPL (requested privilege level).
    ltr ax
    ret
