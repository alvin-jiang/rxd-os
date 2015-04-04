;
; @file: kernel/sys_call.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-28
;

extern _tss


extern exception_handler
extern intcb_table, int_reenter, current_task

global enable_int, disable_int
global back_to_user_mode
; interrupt
global _hint32_clock
; exception
global divide_error, debug, nmi, breakpoint, overflow, bounds, invalid_op
global device_not_available, double_fault, coprocessor_segment_overrun
global invalid_TSS, segment_not_present, stack_segment, general_protection
global _hint14_page_fault, reserved, coprocessor_error

KERNEL_STACK_TOP    equ 0x90000
PAGE_SIZE           equ 4096
GDT_IDX_FIRST_LDT   equ 5
RTS_IDX_RETADDR     equ 48

INT_M_CTLMASK equ 0x21
INT_S_CTLMASK equ 0xa1

[SECTION .text]
ALIGN 4
[BITS 32]

back_to_user_mode:
    mov esp, [current_task]     ; esp -> task_struct

    ; load ldt
    ; ldt index = GDT_IDX_FIRST_LDT + pid
    mov eax, [esp]
    add eax, GDT_IDX_FIRST_LDT
    shl eax, 3
    lldt ax

    add esp, 8                  ; esp -> rts

    ; DEBUG
    mov ax, 24
    mov gs, ax
    mov byte [gs:80], 'k'
    mov byte [gs:84], 'U'
    inc byte [gs:86]

    ; set tss esp0 -> rts
    ; so CPU know where to store regs during interrupt
    lea eax, [esp + 72]
    mov [_tss + 4], eax
back_to_user_mode_reenter:
    dec dword [int_reenter]
    pop gs
    pop fs
    pop es
    pop ds
    popad
    ; skip retaddr
    add esp, 4
    ; ring0 -> ring3, trigger stack switch
    iretd

; save regs & switch to kernel stack (set esp)
enter_kernel_mode:
    pushad
    push ds
    push es
    push fs
    push gs

    mov dx, ss
    mov ds, dx
    mov es, dx
    mov fs, dx
    ; mov gs, dx
    mov esi, esp                ; esi -> rts

    inc     dword [int_reenter]
    cmp     dword [int_reenter], 0
    jne     .already_in_kernel_mode
    ; set kernel stack pointer
    mov     eax, [current_task]
    add     eax, PAGE_SIZE
    mov     esp, eax
    push    back_to_user_mode
    jmp     [esi + RTS_IDX_RETADDR]
.already_in_kernel_mode:
    push    back_to_user_mode_reenter
    mov byte [gs:100], 'R'  ; DEBUG
    inc byte [gs:102]       ; DEBUG
    jmp     [esi + RTS_IDX_RETADDR]


%macro  HINT_MASTER 1
    call    enter_kernel_mode
    mov ax, 24              ; DEBUG
    mov gs, ax
    mov byte [gs:80], 'K'
    mov byte [gs:84], 'u'
    inc byte [gs:82]
    in  al, 0x21            ; disable current interrupt
    or  al, (1 << %1)
    out 0x21, al
    mov al, 0x20            ; send EOI
    out 0x20, al

    sti
    push %1
    call    [intcb_table + 4 * %1]
    pop ecx             ; TODO: store interrupt number in ECX ??? why?
    cli

    in  al, 0x21            ; enable current interrupt
    and al, ~(1 << %1)
    out 0x21, al
    ; return to back_to_user_mode or back_to_user_mode_reenter,
    ; depending on "int_reenter"
    ret
%endmacro

; Interrupt routines
ALIGN   16
_hint32_clock:        ; irq 0 (the clock)
    HINT_MASTER 0

ALIGN   16
hwint01:        ; irq 1 (keyboard)
    HINT_MASTER    1

ALIGN   16
hwint02:        ; irq 2 (cascade!)
    HINT_MASTER    2

ALIGN   16
hwint03:        ; irq 3 (second serial)
    HINT_MASTER    3

ALIGN   16
hwint04:        ; irq 4 (first serial)
    HINT_MASTER    4

ALIGN   16
hwint05:        ; irq 5 (XT winchester)
    HINT_MASTER    5

ALIGN   16
hwint06:        ; irq 6 (floppy)
    HINT_MASTER    6

ALIGN   16
hwint07:        ; irq 7 (printer)
    HINT_MASTER    7

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

_hint14_page_fault:

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



disable_int:
        mov     ecx, [esp + 4]          ; irq
        pushf
        cli
        mov     ah, 1
        rol     ah, cl                  ; ah = (1 << (irq % 8))
        cmp     cl, 8
        jae     disable_8               ; disable irq >= 8 at the slave 8259
disable_0:
        in      al, INT_M_CTLMASK
        test    al, ah
        jnz     dis_already             ; already disabled?
        or      al, ah
        out     INT_M_CTLMASK, al       ; set bit at master 8259
        popf
        mov     eax, 1                  ; disabled by this function
        ret
disable_8:
        in      al, INT_S_CTLMASK
        test    al, ah
        jnz     dis_already             ; already disabled?
        or      al, ah
        out     INT_S_CTLMASK, al       ; set bit at slave 8259
        popf
        mov     eax, 1                  ; disabled by this function
        ret
dis_already:
        popf
        xor     eax, eax                ; already disabled
        ret

enable_int:
    mov     ecx, [esp + 4]          ; irq
    pushf
    cli
    mov     ah, ~1
    rol     ah, cl                  ; ah = ~(1 << (irq % 8))
    cmp     cl, 8
    jae     enable_8                ; enable irq >= 8 at the slave 8259
enable_0:
    in      al, INT_M_CTLMASK
    and     al, ah
    out     INT_M_CTLMASK, al       ; clear bit at master 8259
    popf
    ret
enable_8:
    in      al, INT_S_CTLMASK
    and     al, ah
    out     INT_S_CTLMASK, al       ; clear bit at slave 8259
    popf
    ret



