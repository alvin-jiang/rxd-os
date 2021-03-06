;
; @file: boot.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-20
;

; Task of boot.asm:
; 0. get root device params
; 1. load setup.bin into memory
; 2. load kernel.bin into memory
; 3. if boot with floppy disk:
;       kill_motor
;    if boot with hard disk:
;       ...
;    if boot with U-disk:
;       ...
; 4. jump to setup.bin start point and execute

; What happens before boot.bin?
; 1. BIOS finish its job, then load first sector(boot.bin) of identified
;   boot device into RAM, starting from physical address 0x00007c00
; 2. jump to 0x00007c00 and execute

%include "const.inc"

    ; move self from 0x7c00 -> 0x90000
    mov ax, 0x07c0
    mov ds, ax
    mov ax, INIT_SEGMENT
    mov es, ax
    xor si, si
    xor di, di
    mov cx, 256
    cld
    rep movsw       ; ds:si -> es:di
    jmp word INIT_SEGMENT:INIT_START;

INIT_START:
    ; Our journey starts here!
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xf000  ; 0x9f000 < 0x9fc00

    ; show "start booting..."
    call    _func_clear_screen
    mov bp, Msg_Boot
    call    _func_disp_msg

    ; 0. get root device params
    call    _func_get_boot_dev_params

    ; 1. load setup.bin into memory
    ; show "loading setup..."
    push es
    mov bp, Msg_Setup
    call    _func_disp_msg
    mov ax, SETUP_SEGMENT
    mov es, ax
    mov bx, 0
    mov ax, 1
    mov cl, [bSetupBinSize]
    call    _func_read_boot_dev

    ; 2. load kernel.bin into memory
    ; show "loading kernel..."
    mov bp, Msg_Kernel
    call    _func_disp_msg
    mov ax, KERNEL_SEGMENT0
    mov es, ax
    mov bx, 0
    mov al, [bSetupBinSize]
    mov ah, 0
    inc ax
    mov cl, [bKernelBinSize]
    call    _func_read_boot_dev
    pop es

    ; 3.
    ; if boot with floppy disk:
    call    _func_kill_motor
    ; if boot with hard disk:
    ;   ...
    ; if boot with U-disk:
    ;   ...

    ; 4. jump to setup.bin start point

    jmp SETUP_SEGMENT:0000h   ; jump to setup.bin!



;-------------------------------------
; Variables
;-------------------------------------
; used in _func_disp_msg()
bPrintToLine        db  0 ; print to which line?
MSG_LEN             equ 17
Msg_Boot:           db  "booting...       "
Msg_Setup:          db  "loading setup... "
Msg_Kernel:         db  "loading kernel..."

; used in _func_read_boot_dev()
bSecReadCnt         db 0
; root device params
ROOT_DEV            equ 0x00    ; first floppy disk (0x80 for first hard disk)
; bRootDevNum         db 0 ; driver number of root device
bSecPerTrk          db 0 ; how many sectors per track?
bHeads              db 0 ; how many heads?

;-------------------------------------
; Functions
;-------------------------------------
_func_clear_screen:
    mov ax, 0600h       ; AH - 06h, Scroll Up Window
                        ; AL - 00h, blank window
    mov bx, 0700h       ; BH - 07h, black background, white character
    mov cx, 0           ; CH - 00h, top
                        ; CL - 00h, left
    mov dx, 0184fh      ; DH - 18h, bottom
                        ; DL - 4fh, right
    int 10h             ; BIOS video service
    ret

_func_disp_msg:
; in:
;   es:bp - Message Offset

    mov ax, cs
    mov es, ax

    mov ax, 01301h              ; AH - 13h, Write String
                                ; AL - 01h, update cursor
    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov cx, MSG_LEN          ; CX - number of chars to write
    mov dh, byte [bPrintToLine] ; DH - row to write
    mov dl, 00h                 ; DL - column to write
    inc byte [bPrintToLine]

    int 10h                     ; BIOS video service
    ret

_func_get_boot_dev_params: ; support floppy & hard disk
    mov ah, 08h                 ; AH - 08h, Get Drive Parameters
    mov dl, ROOT_DEV            ; DL - driver number (bit 7 set for hard disk)
    int 13h                     ; BIOS low level disk services
    jc _func_get_boot_dev_params
    mov [bSecPerTrk], cl
    inc dh
    mov [bHeads], dh
    ret

_func_read_boot_dev:
; NOTE: if sector size is 512B, max read data size is 512 * 255 = 127.5KB
; in:
;   ax - Start Sector(LBA)
;   cl - Count to Read(1-255)
;   es:bx - Result Output

    push es
.next_read:
    push ax ; save start sector
    mov [bSecReadCnt], cl

    mov dl, [bSecPerTrk]
    div dl

    ; sector number = LBA % bSecPerTrk + 1
    mov cl, ah
    inc cl                      ; CL - sector number 1-63 (bits 0-5), high two bits of cylinder (bits 6-7, hard disk only)

    mov ah, 0
    mov dl, [bHeads]
    div dl

    ; cylinder number = (LBA / bSecPerTrk) / bHeads
    mov ch, al                  ; CH - low eight bits of cylinder number
    ; head number = (LBA / bSecPerTrk) % bHeads
    mov dh, ah                  ; DH - head number

    mov dl, ROOT_DEV            ; DL - drive number (bit 7 set for hard disk)

    mov al, [bSecReadCnt]       ; AL - number of sectors to read (>0)
    cmp al, 64 ; 64 sectors max per read
    jbe .do_read
    mov al, 64

.do_read:
    mov ah, 02h                 ; AH - 02h, Read Sector(s) Into Memory
    int 13h                     ; BIOS low level disk services
    jc .do_read

    sub byte [bSecReadCnt], al
    mov cl, [bSecReadCnt]   ; set next read count
    movzx ax, al
    pop dx ; restore start sector
    add ax, dx              ; set next read start sector
    mov dx, es
    add dx, 0x800
    mov es, dx
    
    cmp cl, 0
    jne .next_read

    pop es
    ret
    ; reset read
    ;mov ah, 00h
    ;int 13h

_func_kill_motor: ; kill floppy disk motor
    push dx
    mov dx, 03F2h
    mov al, 0
    out dx, al
    pop dx
    ret


;-------------------------------------
; Special Params
;-------------------------------------
times   508-($-$$)  db  0   ; fill bytes

; Note: these two params are automatically filled by make!
bSetupBinSize       db  0   ; how many sectors setup.bin takes?
bKernelBinSize      db  0   ; how many sectors kernel.bin takes?

;-------------------------------------
; End of Boot Sector
;-------------------------------------
dw      0xaa55              ; end flag, last 2 bytes of boot sector(511-512)
