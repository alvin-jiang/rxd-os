;
; @file: kernel/sys_call.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-28
;

extern current_task
global move_to_user_mode

[SECTION .text]
ALIGN 4
[BITS 32]

move_to_user_mode:
    mov ax, 0x20
    ltr ax

ret_from_sys_call:
    ; load ldt
    mov ax, 0x28
    lldt ax

    mov eax, [current_task]
    add eax, 4
    mov esp, eax
    pop gs
    pop fs
    pop es
    pop ds
    popad

    ; skip retaddr
    add esp, 4
    ; ring0 -> ring3, trigger stack switch
    iretd

