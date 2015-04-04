#include "head.h"

extern struct desc_struct *_gdt;
extern struct gate_struct *_idt;
extern struct tss_struct *_tss;
struct desc_struct *gdt = (struct desc_struct *)&_gdt;
struct gate_struct *idt = (struct gate_struct *)&_idt;
struct tss_struct *tss = (struct tss_struct *)&_tss;

int_callback intcb_table[INT_REQ_NR];
int int_reenter;

void set_int_callback(int int_nr, int_callback hdl) {
    intcb_table[int_nr] = hdl;
}