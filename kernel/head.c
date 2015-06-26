#include "head.h"

#include "assert.h"

extern void *_gdt;
extern void *_idt;
extern void *_tss;
struct desc_struct *gdt = (struct desc_struct *)&_gdt;
struct gate_struct *idt = (struct gate_struct *)&_idt;
struct tss_struct *tss = (struct tss_struct *)&_tss;

void on_unknown_irq (int irq) {
    printf("unknown irq: %d\n", irq);
    while(1);
}
irq_handler irqhdl_table[IRQ_NR] = {
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
    on_unknown_irq,
};

void set_desc_base (struct desc_struct *p_desc, u32 base)
{
    p_desc->low &= 0x00ff;
    p_desc->low |= base << 16;
    p_desc->high &= 0x00ffff00;
    p_desc->high |= (base >> 16 & 0xff) | (base & 0xff000000);
}
void set_desc_limit (struct desc_struct *p_desc, u32 limit)
{
    limit &= 0xfffff;
    p_desc->low = (p_desc->low & 0xffff0000) | (limit & 0x0000ffff);
    p_desc->high = (p_desc->high & 0xfff0ffff) | (limit & 0xf0000);
}

void set_irq_handler(int irq_nr, irq_handler hdl)
{
    irqhdl_table[irq_nr] = hdl;
}

void enable_irq (int irq_nr)
{
    assert(irq_nr >= 0 && irq_nr < 16);

    u8 bit = 1 << (irq_nr % 8);
    klock();
    if (irq_nr < 8 && (inb(0x21) & bit)) {
        outb(0x21, inb(0x21) & ~(bit));
    }
    else if (irq_nr >= 8 && (inb(0xa1) & bit)) {
        outb(0xa1, inb(0xa1) & ~(bit));
    }
    kunlock();
}

void disable_irq (int irq_nr)
{
    assert(irq_nr >= 0 && irq_nr < 16);

    u8 bit = 1 << (irq_nr % 8);
    klock();
    if (irq_nr < 8 && !(inb(0x21) & bit)) {
        outb(0x21, inb(0x21) | bit);
    }
    else if (irq_nr >= 8 && !(inb(0xa1) & bit)) {
        outb(0xa1, inb(0xa1) | bit);
    }
    kunlock();
}
