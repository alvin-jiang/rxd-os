;
; @file: kernel/system.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-28
;

extern printk
global memcpy_from_user, memcpy_to_user, strncpy_from_user

; vars for user-mode & kernel-mode switch
extern _tss, current_task
global move_to_user_mode, ret_from_fork

; interrupt & syscall
extern syscall_table, irqhdl_table
; handlers
global _hint144_sys_call, _hint14_page_fault
global _hint32_clock, _hint33_keyboard, _hint34_cascade, _hint46_AT
; callbacks
extern do_no_page, do_wp_page

; ignored
extern exception_handler
global divide_error, debug, nmi, breakpoint, overflow, bounds, invalid_op
global device_not_available, double_fault, coprocessor_segment_overrun
global invalid_TSS, segment_not_present, stack_segment, general_protection
global reserved, coprocessor_error

PAGE_SIZE           equ 4096
GDT_IDX_FIRST_LDT   equ 5

OFFSET_PID          equ 0
OFFSET_THREAD_EIP   equ 4
OFFSET_THREAD_ESP   equ 8

INT_M_CTLMASK equ 0x21
INT_S_CTLMASK equ 0xa1

[SECTION .text]
ALIGN 4
[BITS 32]

move_to_user_mode:
    ; set task kernel stack, tss.esp0 = current_task + PAGE_SIZE
    mov esi, [current_task]
    mov [_tss + 4], esi
    add word [_tss + 4], PAGE_SIZE

    ; switch memory
    ; load ldt, ldt index = pid + first_ldt
    mov eax, [esi + OFFSET_PID]
    add eax, GDT_IDX_FIRST_LDT
    shl eax, 3
    lldt ax

    ; set return address
    push 0x0f               ; ss
    push dword [esi + OFFSET_THREAD_ESP]    ; esp
    push 0x3200             ; eflags
    push 0x07               ; cs
    push dword [esi + OFFSET_THREAD_EIP]    ; eip

    ; restore regs
    mov ax, 0x1b
    mov gs, ax
    mov ax, 0x0f
    mov fs, ax
    mov es, ax
    mov ds, ax
    mov edi, 0
    mov esi, 0
    mov ebp, 0
    mov ebx, 0
    mov edx, 0
    mov ecx, 0
    mov eax, 0

    ; return to user mode
    iretd

; void memcpy_from_user (char * kaddr, const char * uaddr, int count)
memcpy_from_user:
    push ebp
    mov ebp, esp
    push esi
    push edi

    ; fs:esi -> es:edi
    mov edi, [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]
.loop:
    cmp ecx, 0
    je .end
    mov al, [fs:esi]
    mov [es:edi], al
    inc edi
    inc esi
    dec ecx
    jmp .loop
.end:
    pop edi
    pop esi
    pop ebp
    ret

; void memcpy_to_user (char * uaddr, const char * kaddr, int count)
memcpy_to_user:
    push ebp
    mov ebp, esp
    push esi
    push edi

    ; ds:esi -> fs:edi
    mov edi, [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]
.loop:
    cmp ecx, 0
    je .end
    mov al, [ds:esi]
    mov [fs:edi], al
    inc edi
    inc esi
    dec ecx
    jmp .loop
.end:
    pop edi
    pop esi
    pop ebp
    ret

; void strncpy_from_user(char * kaddr, const char * uaddr, int count)
strncpy_from_user:
    push ebp
    mov ebp, esp
    push esi
    push edi

    ; fs:esi -> es:edi
    mov edi, [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]
.loop:
    cmp ecx, 0
    je .end
    mov al, [fs:esi]
    cmp al, 0
    je .pad_zero
    mov [es:edi], al
    inc edi
    inc esi
    dec ecx
    jmp .loop
.pad_zero:
    cmp ecx, 0
    je .end
    mov byte [es:edi], 0
    inc edi
    dec ecx
    jmp .pad_zero
.end:
    pop edi
    pop esi
    pop ebp
    ret


%macro SAVE_ALL 0
    pushad
    push ds
    push es
%endmacro

%macro RESTORE_ALL 0
    pop es
    pop ds
    popad
%endmacro

%macro SYS_ENTER 0
    mov bp, ss
    mov ds, bp
    mov es, bp
%endmacro

; system call
_hint144_sys_call:
    SAVE_ALL
    SYS_ENTER

    ; push params
    push edx
    push ecx
    push ebx
    call    [syscall_table + eax * 4]
    add esp, 12
    mov [esp + 9 * 4], eax  ; set return value

    RESTORE_ALL
    iretd

ret_from_fork:
    RESTORE_ALL
    iretd

; irq
%macro  HINT_MASTER 1
    SAVE_ALL
    SYS_ENTER

    in  al, 0x21            ; disable current interrupt
    or  al, (1 << %1)
    out 0x21, al
    mov al, 0x20            ; send EOI
    out 0x20, al

    ;sti
    push %1
    call    [irqhdl_table + 4 * %1]
    add esp, 4
    ;cli

    in  al, 0x21            ; enable current interrupt
    and al, ~(1 << %1)
    out 0x21, al

    RESTORE_ALL
    iretd
%endmacro

%macro  HINT_SLAVE 1
    SAVE_ALL
    SYS_ENTER

    in  al, 0xa1            ; disable current interrupt
    or  al, (1 << (%1 - 8))
    out 0xa1, al
    mov al, 0x20            ; send EOI
    out 0x20, al
    nop
    out 0xa0, al

    ;sti
    push %1
    call    [irqhdl_table + 4 * %1]
    add esp, 4
    ;cli

    in  al, 0xa1            ; enable current interrupt
    and al, ~(1 << (%1 - 8))
    out 0xa1, al

    RESTORE_ALL
    iretd
%endmacro

; Interrupt request routines 32~47
ALIGN   16
_hint32_clock:  ; irq 0 (the clock)
    SAVE_ALL
    SYS_ENTER

    mov al, 0x20            ; send EOI
    out 0x20, al

    push 0
    call    [irqhdl_table + 4 * 0]
    add esp, 4

    RESTORE_ALL
    iretd

ALIGN   16
_hint33_keyboard:
HINT_MASTER 1   ; irq 1 (keyboard)

ALIGN   16
_hint34_cascade:
HINT_MASTER 2   ; irq 2 (cascade!)

ALIGN   16
HINT_MASTER 3   ; irq 3 (second serial)

ALIGN   16
HINT_MASTER 4   ; irq 4 (first serial)

ALIGN   16
HINT_MASTER 5   ; irq 5 (XT winchester)

ALIGN   16
HINT_MASTER 6   ; irq 6 (floppy)

ALIGN   16
HINT_MASTER 7   ; irq 7 (printer)

ALIGN   16
HINT_SLAVE  8   ; irq 8 (realtime clock).

ALIGN   16
HINT_SLAVE  9   ; irq 9 (irq 2 redirected)

ALIGN   16
HINT_SLAVE  10  ; irq 10

ALIGN   16
HINT_SLAVE  11  ; irq 11

ALIGN   16
HINT_SLAVE  12  ; irq 12

ALIGN   16
HINT_SLAVE  13  ; irq 13 (FPU exception)

ALIGN   16
_hint46_AT:
HINT_SLAVE  14  ; irq 14 (AT winchester)

ALIGN   16
HINT_SLAVE  15  ; irq 15


; Exception routines
divide_error:
    push 0x0
    push 0
    jmp _exception
debug:
    push 0x0
    push 1
    jmp _exception
nmi:
    push 0x0
    push 2
    jmp _exception
breakpoint:
    push 0x0
    push 3
    jmp _exception
overflow:
    push 0x0
    push 4
    jmp _exception
bounds:
    push 0x0
    push 5
    jmp _exception
invalid_op:
    push 0x0
    push 6
    jmp _exception
device_not_available:
    push 0x0
    push 7
    jmp _exception
double_fault:
    push 8
    jmp _exception
coprocessor_segment_overrun:
    push 0x0
    push 9
    jmp _exception
invalid_TSS:
    push 10
    jmp _exception
segment_not_present:
    push 11
    jmp _exception
stack_segment:
    push 12
    jmp _exception
general_protection:
    push 13
    jmp _exception

_hint14_page_fault:
    xchg [esp], eax
    push ecx
    push edx
    push ebx
    sub esp, 4
    push ebp
    push esi
    push edi
    push ds
    push es

    SYS_ENTER

    mov edx, cr2        ; error address
    push edx
    push eax            ; error code
    test eax, 1
    jnz .1
    call    do_no_page
    jmp .2
.1:
    call    do_wp_page
.2:
    add esp, 8

    RESTORE_ALL
    iretd

reserved:
    push 15
    jmp _exception
coprocessor_error:
    push 16
    jmp _exception

_exception:
    call    exception_handler
    add esp, 8
    jmp $

