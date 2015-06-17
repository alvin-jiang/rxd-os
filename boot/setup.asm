;
; @file: setup.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-23
;

; Task of setup.bin:
; 1. get params: display, memory, hd
; 2. cli
; 4. move kernel to memory 0x00000
; 5. fix 8259A
; 6. set temporary GDT, A20, enable Protected-Mode
; 7. jump to head part of kernel.bin

%include "const.inc"
%include "pm.inc"

SETUP_START:

; read & save cursor pos
    mov ah, 03h
    mov bh, 0
    int 10h
    mov dl, 0
    inc dh
    mov word [0], dx

    mov bp, Msg_Setup
    call    PrintLocalMsg
    mov bp, Msg_CheckParams
    call    PrintLocalMsg
    mov bp, Msg_MemSize
    call    PrintLocalMsg

; read & save extend memory size
    call    _func_check_mem
    mov word [2], ax
    mov word [4], bx

    mov bp, 4
    call    PrintWord
    mov bp, 2
    call    PrintWord

; read & save floppy & hd params
check_hd:
    mov ah, 08h
    mov dl, 80h
    int 13h
    jc check_hd
    mov [6], cl
    inc dh
    mov [7], dh
;check_floppy:
;    mov ah, 08h
;    mov dl, 00h
;    int 13h
;    jc check_floppy
;    mov [10], cl
;    inc dh
;    mov [12], dh

; reset segment regs
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xf000 ; 0x9f200 < 0x9fc00

; disable interrupt
; we can't use BIOS functions after this!
; after enter protected-mode, we can use printk
    cli

; move kernel to memory 0x00000
    push ds
    push es

    mov ax, INIT_SEGMENT
    mov ds, ax
    mov ax, 0
    mov al, [ds:509]
    shl ax, 8       ; ax = kernel size in words

    mov bx, KERNEL_SEGMENT0
    mov ds, bx
    mov bx, 0x0000
    mov es, bx
    cld
.move_kernel:
    cmp ax, 0
    jz .move_kernel_end
    mov cx, ax
    cmp cx, 0x4000  ; 16K words = 32KB
    jbe .move_1
    mov cx, 0x4000
.move_1:
    sub ax, cx
    xor si, si
    xor di, di
    rep movsw       ; ds:si -> es:di
    mov bx, ds
    add bx, 0x800
    mov ds, bx
    mov bx, es
    add bx, 0x800
    mov es, bx
    jmp .move_kernel
.move_kernel_end:

    pop es
    pop ds

; fix 8259A compatibility problems
    call    _func_reset_8259A

; load temporary GDT, open A20, enter protected-mode
    ; load temporary GDT, so we can enter protected-mode
    ; and jump to head.asm, we will reload GDT in there
    lgdt [_tmp_gdtr]
    ; enable A20, so we can access memory >= 1 MB
    in  al, 92h
    or  al, 00000010b
    out 92h, al
    ; set CR0, after this, CPU operates in protected-mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

; jump to kernel
    ; note address translation has already changed from now,
    ; also note this jmp is a 32-bit instruction in 16-bit code segment.
    jmp dword _tmp_sel_code:0x0000

;-------------------------------------
; Variables
;-------------------------------------
; used in PrintLocalMsg()
Msg_Setup:          db  "setup...", 0
Msg_CheckParams:    db  "check system params...", 0
Msg_MemSize:        db  "extend memory size = 0x", 0

;-------------------------------------
; Temporary GDT
;-------------------------------------
ALIGN 8
_tmp_gdt:
_tmp_gd_null:   Descriptor  0,        0, 0
_tmp_gd_code:   Descriptor  0,  0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K
_tmp_gd_data:   Descriptor  0,  0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K

_tmp_gdtr   dw  3 * 8 - 1
            dd  SETUP_SEGMENT * 16 + _tmp_gdt

_tmp_sel_code   equ _tmp_gd_code - _tmp_gdt
_tmp_sel_data   equ _tmp_gd_data - _tmp_gdt

;-------------------------------------
; Functions
;-------------------------------------
_func_check_mem:
; out:
;   ax - low 16-bit of extend memory size in KB
;   bx - high 16-bit

    mov ax, 0e801h
    int 15h
    jc .error
    test ax, ax
    jz .error

    test bx, bx
    jz .end
    mov cx, bx
    and cx, 03ffh
    shl cx, 6
    add cx, ax
    mov ax, cx
    shr bx, 10
    jnc .end
    inc bx
.end:
    ret
.error:
    jmp $

_func_reset_8259A:
    mov al, 011h    ; initialization sequence
    out 020h, al    ; send it to 8259A-1
    call    io_delay
    out 0A0h, al    ; and to 8259A-2
    call    io_delay

    mov al, 020h    ; start of hardware int's (IR0 = 0x20)
    out 021h, al
    call    io_delay

    mov al, 028h    ; start of hardware int's 2 (IR8 = 0x28)
    out 0A1h, al
    call    io_delay

    mov al, 004h    ; IR2 = interrupt from 8259A-2
    out 021h, al
    call    io_delay
    mov al, 002h
    out 0A1h, al
    call    io_delay

    mov al, 001h    ; 8086 mode for both
    out 021h, al
    call    io_delay
    out 0A1h, al
    call    io_delay

    mov    al, 0xff ; mask off all interrupts for now
    out 021h, al
    call    io_delay
    out 0A1h, al
    call    io_delay

    ret

io_delay:
    nop
    nop
    nop
    nop
    ret

PrintLocalMsg:
; in:
;   bp - String Offset

    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov dh, [1]                 ; DH - row of cursor (00h is top)
    inc dh
    mov dl, 0                   ; DL - col of cursor (00h is left)
.next_char:
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    int 10h
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp + 0x200]        ; AL - character to write
    test al, al
    jz .end
    mov cx, 1                   ; CX - number of times to write
    int 10h
    inc bp
    inc dl
    jmp .next_char
.end:
    mov word [0], dx
    ret

PrintWord:
; in:
;   bp - Word to Display

    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov dx, word [0]            ; DH - row of cursor (00h is top)
                                ; DL - col of cursor (00h is left)
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    int 10h

    inc bp
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp]                ; AL - character to write
    shr al, 4
    add al, '0'
    cmp al, '9'
    jle disp_word_2
    add al, 7
disp_word_2:
    mov cx, 1                   ; CX - number of times to write
    int 10h
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    inc dl
    int 10h

    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp]                ; AL - character to write
    and al, 0fh
    add al, '0'
    cmp al, '9'
    jle disp_word_1
    add al, 7
disp_word_1:
    mov cx, 1                   ; CX - number of times to write
    int 10h
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    inc dl
    int 10h

    dec bp
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp]                ; AL - character to write
    shr al, 4
    add al, '0'
    cmp al, '9'
    jle disp_word_4
    add al, 7
disp_word_4:
    mov cx, 1                   ; CX - number of times to write
    int 10h
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    inc dl
    int 10h

    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp]                ; AL - character to write
    and al, 0fh
    add al, '0'
    cmp al, '9'
    jle disp_word_3
    add al, 7
disp_word_3:
    mov cx, 1                   ; CX - number of times to write
    int 10h
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    inc dl
    int 10h

    mov word [0], dx
    ret


