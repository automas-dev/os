[bits 32]

; If this file moves, update the comment in include/process.h

[extern tss_set_esp0]
[extern tss_get_esp0]

; These must match struct _process in src/kernel/include/process.h
TCB_CR3      equ 0
TCB_ESP      equ 4
TCB_ESP0     equ 8

; proc_t *
active_task: dd  0

; void set_active_task(proc_t * active)
global set_active_task
set_active_task:
    ; ebp = args
    push ebp,
    mov ebp, esp
    add ebp, 4

    push eax

    ; eax = active
    mov eax,           [ebp+4]
    mov [active_task], eax

    pop eax
    pop ebp

    ret

; proc_t * get_active_task(void)
global get_active_task
get_active_task:
    mov eax, [active_task]

    ret

; switch_task(proc_t * next)
global switch_task
switch_task:
    ; ebp = args
    push ebp
    mov  ebp, esp
    add  ebp, 8

    push edi
    push esi
    push eax

    ; edi = active
    mov edi, [active_task]
    ; esi = next
    mov esi, [ebp]

    ; store cr3
    mov eax,           cr3
    mov [edi+TCB_CR3], eax

    ; store esp
    mov [edi+TCB_ESP], esp

    ; store esp0
    call tss_get_esp0
    mov  [edi+TCB_ESP0], eax

.resume:
    mov [active_task], esi

    ; load esp0
    mov  eax, [esi+TCB_ESP0]
    push eax
    call tss_set_esp0
    pop  eax

    ; load esp
    mov esp, [esi+TCB_ESP]

    ; load cr3
    mov eax, [esi+TCB_CR3]
    mov cr3, eax

    pop eax
    pop esi
    pop edi

    pop ebp

    ret

; start_first_task(proc_t * next)
global start_first_task
start_first_task:
    ; ebp = args
    push ebp
    mov  ebp, esp
    add  ebp, 8

    push edi
    push esi
    push eax

    ; esi = next
    mov esi, [ebp]

    jmp switch_task.resume

; ; void * function
; tmp_jump_target: dd  0

; ; jump_usermode(proc_t * next)
; global jump_usermode
; extern test_user_function
; jump_usermode:
;     ; ebp = args
;     push ebp
;     mov  ebp, esp
;     add  ebp, 8

; 	mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
; 	mov ds, ax
; 	mov es, ax 
; 	mov fs, ax 
; 	mov gs, ax ; SS is handled by iret

; 	; set up the stack frame iret expects
; 	mov eax, esp
; 	push (4 * 8) | 3 ; data selector
; 	push eax ; current esp
; 	pushf ; eflags
; 	push (3 * 8) | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
; 	push [ebp] ; instruction address to return to
; 	iret
