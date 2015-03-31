;
; @file: kernel/sys_call.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-28
;

extern current_task, exception_handler, tss

global enable_int, disable_int

global restart
global divide_error, debug, nmi, breakpoint, overflow, bounds, invalid_op
global device_not_available, double_fault, coprocessor_segment_overrun
global invalid_TSS, segment_not_present, stack_segment, general_protection
global page_fault, reserved, coprocessor_error

KERNEL_STACK_TOP    equ 0x90000
INT_M_CTLMASK equ 0x21
INT_S_CTLMASK equ 0xa1

[SECTION .text]
ALIGN 4
[BITS 32]

restart:
    mov esp, [current_task]     ; exp -> task_struct
    ; load ldt
    mov eax, [esp]
    add eax, 5 ; first ldt pos
    shl eax, 3
    lldt ax
    ; set tss esp0
    add esp, 8                  ; exp -> rts
    lea eax, [esp + 72]
    mov [tss + 4], eax

restart_reenter:
    pop gs
    pop fs
    pop es
    pop ds
    popad

    ; skip retaddr
    add esp, 4
    ; ring0 -> ring3, trigger stack switch
    iretd


save:
    pushad
    push    ds
    push    es
    push    fs
    push    gs

    mov     dx, ss
    mov     ds, dx
    mov     es, dx
    mov     esi, esp

    ;inc     dword [k_reenter]
    ;cmp     dword [k_reenter], 0
    jne     .1
    ;mov     esp, KERNEL_STACK_TOP
    push    restart
    ;jmp     [esi + RETADR - P_STACKBASE]
.1:
    push    restart_reenter
    ;jmp     [esi + RETADR - P_STACKBASE]

%macro  hwint_master    1
    ret
%endmacro

; Interrupt routines
ALIGN   16
hwint00:        ; irq 0 (the clock).
    call    save

    ; disable current irq
    in  al, 0x21
    or  al, 1
    out 0x21, al
    ; send EOI
    mov al, 0x20
    out 0x20, al

    sti
    push    0
    ;call    [irq_table]
    pop ecx
    cli

    ; enable current irq
    in  al, 0x21
    and al, ~(1)
    out 0x21, al
    ret

ALIGN   16
hwint01:        ; irq 1 (keyboard)
    hwint_master    1

ALIGN   16
hwint02:        ; irq 2 (cascade!)
    hwint_master    2

ALIGN   16
hwint03:        ; irq 3 (second serial)
    hwint_master    3

ALIGN   16
hwint04:        ; irq 4 (first serial)
    hwint_master    4

ALIGN   16
hwint05:        ; irq 5 (XT winchester)
    hwint_master    5

ALIGN   16
hwint06:        ; irq 6 (floppy)
    hwint_master    6

ALIGN   16
hwint07:        ; irq 7 (printer)
    hwint_master    7

; ---------------------------------
%macro  hwint_slave 1
    push    %1
    ;call    spurious_int
    add esp, 4
    hlt
%endmacro
; ---------------------------------

ALIGN   16
hwint08:        ; irq 8 (realtime clock).
    hwint_slave 8

ALIGN   16
hwint09:        ; irq 9 (irq 2 redirected)
    hwint_slave 9

ALIGN   16
hwint10:        ; irq 10
    hwint_slave 10

ALIGN   16
hwint11:        ; irq 11
    hwint_slave 11

ALIGN   16
hwint12:        ; irq 12
    hwint_slave 12

ALIGN   16
hwint13:        ; irq 13 (FPU exception)
    hwint_slave 13

ALIGN   16
hwint14:        ; irq 14 (AT winchester)
    hwint_slave 14

ALIGN   16
hwint15:        ; irq 15
    hwint_slave 15

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
page_fault:
    push 14
    jmp _exception
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






