
#ifndef __HEAD_H__
#define __HEAD_H__

#include "type.h"

struct desc_struct
{
    u32     low, high;
};

struct gate_struct
{
    u16     offset_low;
    u16     selector;
    u8      __reserved;   // we don't use call gate
    u8      attr;
    u16     offset_high;
};

struct tss_struct
{
    u32     __reserved0;
    u32     esp0;
    u32     ss0;
    u16     __reserved1[45];
    u16     iobase;
};

extern struct desc_struct *gdt;
extern struct gate_struct *idt;
extern struct tss_struct *tss;

void set_desc_base (struct desc_struct *p_desc, u32 base);
void set_desc_limit (struct desc_struct *p_desc, u32 limit);

/* Boot Params */
#define BOOT_PARAM_CURSOR           (*(u16 *)0x90000)
#define BOOT_PARAM_MEM_SIZE         (*(u32 *)0x90002)
#define BOOT_PARAM_HD_SECT_PER_TRK  (*(u8 *)0x90006)
#define BOOT_PARAM_HD_HEAD_NR       (*(u8 *)0x90007)

/* GDT, LDT */
#define GDT_IDX_FIRST_LDT 5
#define INIT_IDT_DESC {0x0, 0x00408200}

#define _LDT(n) ((((unsigned long) n)<<3)+(GDT_IDX_FIRST_LDT<<3))
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))

/* IDT */
// irq
#define IRQ_NR                  16
#define IRQ0_CLOCK              0
#define IRQ1_KEYBOARD           1
#define IRQ2_8259_SLAVE         2
#define IRQ3_SECOND_SERIAL      3
#define IRQ4_FIRST_SERIAL       4
#define IRQ5_XTWIN              5
#define IRQ6_FLOPPY             6
#define IRQ7_PRINTER            7
#define IRQ8_REALTIME_CLOCK     8
#define IRQ9_8259_MASTER        9
#define IRQ10                   10
#define IRQ11                   11
#define IRQ12                   12
#define IRQ13_FPU_EXCEPTION     13
#define IRQ14_ATWIN             14
#define IRQ15                   15

typedef void (*irq_handler) (int irq_nr);

extern irq_handler irqhdl_table[];
extern void set_irq_handler (int irq_nr, irq_handler hdl);
extern void enable_irq (int irq_nr);
extern void disable_irq (int irq_nr);
// interrupt
// exception
extern void exception_handler(u32 exp_nr, u32 err_code, u32 eip, u32 cs, u32 eflags);
// syscall
typedef void * syscall;
extern syscall syscall_table[];

/* I/O assembly */
#include "asm.h"

#define FASTCALL __attribute__((fastcall))
// #define FASTCALL(func) func __attribute__((regparm(3)))

#endif
