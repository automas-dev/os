[org 0x7c00]

LOAD_ADDR equ 0x7e00
BOOT_DRIVE equ 0x80

start:
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ah, 0x02        ; BIOS read sectors
    mov al, 1           ; one sector
    mov ch, 0           ; cylinder 0
    mov cl, 2           ; sector 1
    mov dh, 0           ; head 0
    mov dl, BOOT_DRIVE  ; drive number
    mov bx, LOAD_ADDR   ; buffer address
    int 0x13

    jc hang

    jmp letter

hang:
    hlt
    jmp hang

; DATA          equ 0x500
; STATE         equ DATA + 0x100
; FIRST_NIBBLE  equ DATA + 0x101

; ; Type this (no spaces) then press enter to see a single A character
; ; b4 0e b0 41 cd 10 f4
; ; That encodes:
; ; b4 0e → mov ah, 0x0e
; ; b0 41 → mov al, 'A'
; ; cd 10 → int 0x10
; ; f4 → hlt

; start:
;     mov si, DATA
;     mov byte [STATE], 0

; .next_key:
;     mov ah, 0x00        ; BIOS: wait for keyboard input
;     int 0x16            ; Read keypress; ASCII char in AL

;     mov ah, 0x0E        ; BIOS teletype output
;     int 0x10            ; Print character in AL

;     cmp al, 0x0D        ; Enter key
;     je .jump_to_data

;     call parse_nibble

;     cmp byte [STATE], 0
;     jne .second_nibble

;     mov byte [FIRST_NIBBLE], al
;     mov byte [STATE], 1
;     jmp .next_key

; .second_nibble:
;     mov byte [STATE], 0
;     mov ah, [FIRST_NIBBLE]
;     shl ah, 4
;     or ah, al
;     mov byte [si], ah
;     inc si
;     jmp .next_key

; .jump_to_data:
;     jmp DATA

; parse_nibble:
;     cmp al, '0'
;     jb .done
;     cmp al, '9'
;     jbe .digit
;     cmp al, 'a'
;     jb .done
;     cmp al, 'f'
;     jbe .letter

; .done:
;     ret

; .digit:
;     sub al, '0'
;     ret

; .letter:
;     sub al, 'a'
;     add al, 10
;     ret

times 510-($-$$) db 0
dw                  0xaa55

letter:
    mov ah, 0x0e
    mov al, 'A'
    int 0x10
    hlt