
#ifndef __HEAD_H__
#define __HEAD_H__

#include "type.h"

struct desc_struct
{
    uint32 low, high;
};

struct gate_struct
{
    uint16    offset_low;
    uint16    selector;
    uint8    __reserved;   // we don't use call gate
    uint8    attr;
    uint16    offset_high;
};

struct tss_struct
{
    uint32 __reserved0;
    uint32 esp0;
    uint32 ss0;
    uint16 __reserved1[45];
    uint16 iobase;
};

// boot params
#define BOOT_PARAM_CURSOR           (*(uint16 *)0x90000)
#define BOOT_PARAM_MEM_SIZE         (*(uint32 *)0x90002)
#define BOOT_PARAM_HD_SECT_PER_TRK  (*(uint8 *)0x90006)
#define BOOT_PARAM_HD_HEAD_NR       (*(uint8 *)0x90007)

#define PAGE_SIZE 4096

// GDT, LDT
#define GDT_IDX_FIRST_LDT 5
#define INIT_IDT_DESC {0x0, 0x00408200}

// IDT: Interrupt, Exception, Syscall
#define INT_REQ_NR 16
// #define NR_INT_CLOCK            0x20

#define EXCEPTION_DIVIDE        0x0
#define EXCEPTION_DEBUG         0x1
#define EXCEPTION_NMI           0x2
#define EXCEPTION_BREAKPOINT    0x3
#define EXCEPTION_OVERFLOW      0x4
#define EXCEPTION_BOUNDS        0x5
#define EXCEPTION_INVAL_OP      0x6
#define EXCEPTION_COPROC_NOT    0x7
#define EXCEPTION_DOUBLE_FAULT  0x8
#define EXCEPTION_COPROC_SEG    0x9
#define EXCEPTION_INVAL_TSS     0xA
#define EXCEPTION_SEG_NOT       0xB
#define EXCEPTION_STACK_FAULT   0xC
#define EXCEPTION_PROTECTION    0xD
#define EXCEPTION_PAGE_FAULT    0xE
#define EXCEPTION_COPROC_ERR    0x10

extern struct desc_struct *gdt;
extern struct gate_struct *idt;
extern struct tss_struct *tss;

/* interrupt */
typedef void (*int_callback) (int int_nr);
extern int_callback intcb_table[];
extern int int_reenter;
/* syscall */
typedef void * syscall;
extern syscall syscall_table[];

// head.c
void set_int_callback (int int_nr, int_callback hdl);
void set_desc_base (struct desc_struct *p_desc, uint32 base);
void set_desc_limit (struct desc_struct *p_desc, uint32 limit);
// system.asm
void enable_int (int int_nr);
void disable_int (int int_nr);
uint8 in_byte (uint16 port);
void out_byte (uint16 port, uint8 value);
#define sti() __asm__ ("sti"::)
#define cli() __asm__ ("cli"::)
void port_read (uint16 port, void * buf, int n);
void port_write (uint16 port, void * buf, int n);
void port_write2 (uint16 port, void * buf, int n);

#define _LDT(n) ((((unsigned long) n)<<3)+(GDT_IDX_FIRST_LDT<<3))
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))

#define FASTCALL __attribute__((fastcall))
// #define FASTCALL(func) func __attribute__((regparm(3)))

// TODO: make this asm macro
// void set_base(struct desc_struct *p_desc, uint32 base);
// void set_limit(struct desc_struct *p_desc, uint32 limit);
// #define set_base(addr, base) ()
// #define set_limit(addr, limit) ()




// /* 中断向量 */
// #define INT_VECTOR_DIVIDE       0x0
// #define INT_VECTOR_DEBUG        0x1
// #define INT_VECTOR_NMI          0x2
// #define INT_VECTOR_BREAKPOINT       0x3
// #define INT_VECTOR_OVERFLOW     0x4
// #define INT_VECTOR_BOUNDS       0x5
// #define INT_VECTOR_INVAL_OP     0x6
// #define INT_VECTOR_COPROC_NOT       0x7
// #define INT_VECTOR_DOUBLE_FAULT     0x8
// #define INT_VECTOR_COPROC_SEG       0x9
// #define INT_VECTOR_INVAL_TSS        0xA
// #define INT_VECTOR_SEG_NOT      0xB
// #define INT_VECTOR_STACK_FAULT      0xC
// #define INT_VECTOR_PROTECTION       0xD
// #define INT_VECTOR_PAGE_FAULT       0xE
// #define INT_VECTOR_COPROC_ERR       0x10

// /* 中断向量 */
// #define INT_VECTOR_IRQ0         0x20
// #define INT_VECTOR_IRQ8         0x28

// /* 系统调用 */
// #define INT_VECTOR_SYS_CALL             0x90

#endif
