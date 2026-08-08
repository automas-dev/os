[org 0x7c00]

STACK equ 0x6fff
LOAD_ADDR equ 0x7e00
BOOT_DRIVE equ 0x80

start:
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov bp, STACK
    mov sp, bp

    ; jmp write_to_disk
    jmp read_from_disk

    jc error

    jmp success

hang:
    hlt
    jmp hang

error:
    mov al, 'E'
    call print_c
    jmp hang

success:
    mov al, 'S'
    call print_c
    jmp hang

write_to_disk:
    mov byte [LOAD_ADDR], 0x43  ; 41 = A 42 = B 43 = C
    mov bx, LOAD_ADDR
    mov ax, 0
    mov es, ax
    mov ah, 3
    mov al, 1
    mov cl, 1
    mov bp, 0
    mov dh, 0
    mov dl, BOOT_DRIVE
    int 0x13

    jc error

    jmp success

read_from_disk:
    mov ah, 2           ; BIOS read sectors
    mov al, 1           ; one sector
    mov ch, 0           ; cylinder 0
    mov cl, 1           ; sector 1
    mov dh, 0           ; head 0
    mov dl, BOOT_DRIVE  ; drive number
    mov bx, LOAD_ADDR   ; buffer address
    int 0x13

    jc error

    ; mov ah, 0x0e
    ; mov al, [LOAD_ADDR]
    ; int 0x10

    ; jmp success
    jmp LOAD_ADDR

; al = char
print_c:
    pusha
    mov ah, 0x0e
    int 0x10
    popa
    ret

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

; letter:
;     mov ah, 0x0e
;     mov al, 'A'
;     int 0x10
;     hlt