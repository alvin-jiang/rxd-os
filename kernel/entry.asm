;
; @file: kernel/system.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-28
;

extern printk

; vars for user-mode & kernel-mode switch
extern _tss, current_task
global move_to_user_mode, ret_from_fork

; interrupt & syscall
extern syscall_table, intcb_table
global enable_int, disable_int
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

; I/O
global in_byte, out_byte, port_read, port_write, port_write2

KERNEL_STACK_TOP    equ 0x90000
PAGE_SIZE           equ 4096
GDT_IDX_FIRST_LDT   equ 5
RTS_IDX_RETADDR     equ 48

INT_M_CTLMASK equ 0x21
INT_S_CTLMASK equ 0xa1

[SECTION .text]
ALIGN 4
[BITS 32]

move_to_user_mode:
    ; set task kernel stack
    ; tss esp0
    mov esi, [current_task]
    mov [_tss + 4], esi
    add word [_tss + 4], PAGE_SIZE

    ; switch memory
    ; load ldt
    mov eax, [esi]
    add eax, GDT_IDX_FIRST_LDT
    shl eax, 3
    lldt ax

    ; set return address
    push 0x0f               ; ss
    push dword [esi + 12]   ; esp
    push 0x3200             ; eflags
    push 0x07               ; cs
    push dword [esi + 8]    ; eip

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

%macro SAVE_ALL 0
    pushad
    push ds
    push es
    push fs
    push gs
%endmacro

%macro RESTORE_ALL 0
    pop gs
    pop fs
    pop es
    pop ds
    popad
%endmacro

%macro SYS_ENTER 0
    mov bp, ss
    mov ds, bp
    mov es, bp
    mov fs, bp
%endmacro

; system call
_hint144_sys_call:
    SAVE_ALL
    SYS_ENTER

    ; push params
    push edx
    push ebx
    push eax
    call    [syscall_table + ecx * 4]

    add esp, 12
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
    call    [intcb_table + 4 * %1]
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
    call    [intcb_table + 4 * %1]
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
    call    [intcb_table + 4 * 0]
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
    push fs
    push gs

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

; void out_byte(u16 port, u8 value);
out_byte:
    mov edx, [esp + 4]      ; port
    mov al, [esp + 4 + 4]   ; value
    out dx, al
    nop
    nop
    ret

; u8 in_byte(u16 port);
in_byte:
    mov edx, [esp + 4]      ; port
    xor eax, eax
    in  al, dx
    nop
    nop
    ret

; void port_read(u16 port, void* buf, int n);
port_read:
    mov edx, [esp + 4]      ; port
    mov edi, [esp + 4 + 4]  ; buf
    mov ecx, [esp + 4 + 4 + 4]  ; n
    shr ecx, 1
    cld
    rep insw
    ret

; void port_write(u16 port, void* buf, int n);
port_write:
    mov edx, [esp + 4]      ; port
    mov esi, [esp + 4 + 4]  ; buf
    mov ecx, [esp + 4 + 4 + 4]  ; n
    shr ecx, 1
    cld
    rep outsw
    ret

port_write2:
    mov edx, [esp + 4]      ; port
    mov esi, [esp + 4 + 4]  ; buf
    mov ecx, [esp + 4 + 4 + 4]  ; n
    shr ecx, 1
    cld
.loop:
    outsw
    jmp $ + 2
    loop .loop
    ret
