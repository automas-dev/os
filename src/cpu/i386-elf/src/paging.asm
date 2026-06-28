; void mmu_enable_paging(uint32_t addr);
global mmu_enable_paging
mmu_enable_paging:
    push eax
    mov  eax, [esp+8]

    push eax

    call mmu_change_dir

    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax

    pop eax
    pop eax

    ret

; void mmu_disable_paging();
global mmu_disable_paging
mmu_disable_paging:
    push eax

    mov eax, cr0
    and eax, 0x7fffffff
    mov cr0, eax

    pop eax

    ret

; void mmu_paging_enabled();
global mmu_paging_enabled
mmu_paging_enabled:
    push eax

    mov eax, cr0
    shr eax, 31
    and eax, 1

    pop eax

    ret

; void mmu_change_dir(uint32_t addr);
global mmu_change_dir
mmu_change_dir:
    push eax
    mov  eax, [esp+8]

    shr eax, 12
    shl eax, 12
    mov cr3, eax

    pop eax

    ret

; uint32_t mmu_get_curr_dir();
global mmu_get_curr_dir
mmu_get_curr_dir:
    mov eax, cr3
    shr eax, 12
    shl eax, 12

    ret

; void mmu_reload_dir();
global mmu_reload_dir
mmu_reload_dir:
    mov eax, cr3
    mov cr3, eax

    ret
