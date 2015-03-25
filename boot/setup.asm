;
; @file: setup.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-23
;

; Task of setup.bin:
; 1. check memory, get params
; 2. check hd, get params
; 3. cli
; 4. move kernel.bin to memory 0x00000
; 5. set IDT, GDT, A20, 8259A, enable Protected-Mode
; 6. jump to kernel.bin start point and execute
    jmp short SETUP_START

%include    "const.inc"
%include    "pm.inc"

INIT_SEGMENT        equ 0x9000
SETUP_SEGMENT       equ 0x9020
KERNEL_SEGMENT      equ 0x1000

SETUP_START:
    mov ax, cs
    mov ds, ax

    mov bp, Msg_Setup
    call    DispString

    ; 1. check memory, get params
    mov bp, Msg_MemChk
    call    DispString
    call    MemoryCheck
    mov bp, Msg_MemSize
    call    DispString

    mov bp, wEXMemSizeHigh
    call    DispWord
    mov bp, wEXMemSizeLow
    call    DispWord

    ; 2. check hd, get params

    ; 3. cli
    cli

    ; 4. move kernel.bin to memory 0x00000

    ; 5. set IDT, GDT, A20, 8259A, enable Protected-Mode
    ; load GDT
    lgdt    [GdtPtr]
    ; enable A20, so we can access memory >= 1 MB
    in  al, 92h
    or  al, 00000010b
    out 92h, al
    ; set CR0, after this, CPU operates in protected-mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    ;; jump to code for protected-mode
    ; note the address translation has already changed,
    ; also note this jmp is a 32-bit instruction in 16-bit code segment.
    jmp dword SelectorFlatC:(SETUP_SEGMENT * 16 + LABEL_PM_START)


;-------------------------------------
; Variables
wEXMemSizeLow       dw  0 ; extend memory (>= 1 MB) size in KB
wEXMemSizeHigh      dw  0 ; extend memory (>= 1 MB) size in KB

; used in DispString()
bCursorRow          db  3
bCursorCol          db  0
Msg_Setup:          db  "setup...", 0
Msg_MemChk:         db  "check memory...", 0
Msg_MemSize:        db  "extend memory size = 0x", 0
Msg_Ready:          db  "loading kernel...", 0

;-------------------------------------
; Temporary GDT
LABEL_GDT:
LABEL_DESC_NULL:        Descriptor             0,                    0, 0
LABEL_DESC_FLAT_C:      Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K
LABEL_DESC_FLAT_RW:     Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K
LABEL_DESC_VIDEO:       Descriptor       0B8000h,               0ffffh, DA_DRW | DA_DPL3

GdtLen  equ $ - LABEL_GDT
GdtPtr  dw  GdtLen - 1
        dd  SETUP_SEGMENT * 16 + LABEL_GDT

;-------------------------------------
; Temporary GDT Selector
SelectorFlatC       equ LABEL_DESC_FLAT_C   - LABEL_GDT
SelectorFlatRW      equ LABEL_DESC_FLAT_RW  - LABEL_GDT
SelectorVideo       equ LABEL_DESC_VIDEO    - LABEL_GDT + SA_RPL3

;-------------------------------------
; Functions

; @MemoryCheck
MemoryCheck:
    mov ax, 0e801h
    int 15h
    jc memory_check_error
    test ax, ax
    jz memory_check_error

    test bx, bx
    jz memory_check_no_high
    mov cx, bx
    and cx, 03ffh
    shl cx, 6
    add cx, ax
    mov [ds:wEXMemSizeLow], cx
    shr bx, 10
    jnc memory_check_1
    inc bx
memory_check_1:
    mov [ds:wEXMemSizeHigh], bx
    jmp memory_check_end
memory_check_no_high:
    mov [ds:wEXMemSizeLow], ax
    mov word [ds:wEXMemSizeHigh], 0
memory_check_end:
    ret
memory_check_error:
    jmp $

; @DispString
; in:
;   bp - String Offset
DispString:
    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov dh, [ds:bCursorRow]     ; DH - row of cursor (00h is top)
    inc dh
    mov dl, 0                   ; DL - col of cursor (00h is left)
disp_msg_next_char:
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    int 10h
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [ds:bp]             ; AL - character to write
    test al, al
    jz disp_msg_end
    mov cx, 1                   ; CX - number of times to write
    int 10h
    inc bp
    inc dl
    jmp disp_msg_next_char
disp_msg_end:
    mov [ds:bCursorRow], dh
    mov [ds:bCursorCol], dl
    ret

; @DispWord
; in:
;   bp - Word to Display
DispWord:
    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov dh, [ds:bCursorRow]     ; DH - row of cursor (00h is top)
    mov dl, [ds:bCursorCol]     ; DL - col of cursor (00h is left)
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    int 10h

    inc bp
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [ds:bp]             ; AL - character to write
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
    mov al, [ds:bp]             ; AL - character to write
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
    mov al, [ds:bp]             ; AL - character to write
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
    mov al, [ds:bp]             ; AL - character to write
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

    mov [ds:bCursorRow], dh
    mov [ds:bCursorCol], dl
    ret

;-------------------------------------
; Code executes in 386 Protected-Mode

[SECTION .s32]
ALIGN   32
[BITS   32]

LABEL_PM_START:
    mov ax, SelectorVideo
    mov gs, ax
    mov ax, SelectorFlatRW
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, SETUP_STACKTOP

    mov byte [gs:0], 'P'

    jmp $
    ; 6. jump to kernel.bin start point and execute
    jmp SelectorFlatC:KERNEL_START  ; jump to kernel!

;-------------------------------------
; Functions in 386 Protected-Mode


;-------------------------------------
; Data in 386 Protected-Mode

[SECTION .data1]
ALIGN   32

LABEL_DATA:

; 堆栈就在数据段的末尾
times   100h    db  0
SETUP_STACKTOP  equ SETUP_SEGMENT * 16 + $ ; 栈顶

