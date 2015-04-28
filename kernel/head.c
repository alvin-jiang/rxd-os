#include "head.h"

extern struct desc_struct *_gdt;
extern struct gate_struct *_idt;
extern struct tss_struct *_tss;
struct desc_struct *gdt = (struct desc_struct *)&_gdt;
struct gate_struct *idt = (struct gate_struct *)&_idt;
struct tss_struct *tss = (struct tss_struct *)&_tss;

void on_unknown_irq (int irq) {
    printf("unknown irq: %d\n", irq);
    while(1);
}
int_callback intcb_table[INT_REQ_NR] = {
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

void set_int_callback(int int_nr, int_callback hdl) {
    intcb_table[int_nr] = hdl;
}

void set_desc_base (struct desc_struct *p_desc, uint32 base)
{
    p_desc->low &= 0x00ff;
    p_desc->low |= base << 16;
    p_desc->high &= 0x00ffff00;
    p_desc->high |= (base >> 16 & 0xff) | (base & 0xff000000);
}
void set_desc_limit (struct desc_struct *p_desc, uint32 limit)
{
    limit &= 0xfffff;
    p_desc->low = (p_desc->low & 0xffff0000) | (limit & 0x0000ffff);
    p_desc->high = (p_desc->high & 0xfff0ffff) | (limit & 0xf0000);
}
