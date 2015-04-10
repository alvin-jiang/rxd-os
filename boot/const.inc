;-------------------------------------
; memory layout
;-------------------------------------

INIT_SEGMENT        equ 0x9000
SETUP_SEGMENT       equ 0x9020
KERNEL_SEGMENT0     equ 0x1000
; finally, kernel is moved to 0x0000

BOOT_PARAMS         equ 0x90000
KERNEL_STACK_TOP    equ 0x90000

;-------------------------------------
; boot params
;-------------------------------------

BOOT_PARAM_CURSOR   equ 0x90000
BOOT_PARAM_MEM_SIZE equ 0x90002


