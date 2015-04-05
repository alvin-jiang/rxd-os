
#include "head.h"

// void set_idt_desc(BYTE vector, BYTE type, int_handler handler, BYTE privilege)
// {
//     struct gate * p_gate    = &idt[vector];
//     u32 base = (u32)handler;
//     p_gate->offset_low  = base & 0xFFFF;
//     p_gate->selector    = SELECTOR_KERNEL_CS;
//     p_gate->dcount      = 0;
//     p_gate->attr        = type | (privilege << 5);
//     p_gate->offset_high = (base >> 16) & 0xFFFF;
// }



// void trap_init()
// {
    // set_trap_gate(0,&divide_error);
    // set_trap_gate(1,&debug);
    // set_trap_gate(2,&nmi);
    // set_system_gate(3,&breakpoint);   /* int3-5 can be called from all */
    // set_system_gate(4,&overflow);
    // set_system_gate(5,&bounds);
    // set_trap_gate(6,&invalid_op);
    // set_trap_gate(7,&device_not_available);
    // set_trap_gate(8,&double_fault);
    // set_trap_gate(9,&coprocessor_segment_overrun);
    // set_trap_gate(10,&invalid_TSS);
    // set_trap_gate(11,&segment_not_present);
    // set_trap_gate(12,&stack_segment);
    // set_trap_gate(13,&general_protection);
    // set_trap_gate(14,&page_fault);
    // set_trap_gate(15,&reserved);
    // set_trap_gate(16,&coprocessor_error);
    
    // outb_p(inb_p(0x21)&0xfb,0x21);   ; enable 8259A-2
    // outb(inb_p(0xA1)&0xdf,0xA1);     ; FPU exception
// }

void exception_handler(DWORD exp_nr, DWORD err_code, DWORD eip, DWORD cs, DWORD eflags)
{
    int i;

    char * err_desc[64] = {  "#DE Divide Error",
        "#DB RESERVED",
        "—  NMI Interrupt",
        "#BP Breakpoint",
        "#OF Overflow",
        "#BR BOUND Range Exceeded",
        "#UD Invalid Opcode (Undefined Opcode)",
        "#NM Device Not Available (No Math Coprocessor)",
        "#DF Double Fault",
        "    Coprocessor Segment Overrun (reserved)",
        "#TS Invalid TSS",
        "#NP Segment Not Present",
        "#SS Stack-Segment Fault",
        "#GP General Protection",
        "#PF Page Fault",
        "—  (Intel reserved. Do not use.)",
        "#MF x87 FPU Floating-Point Error (Math Fault)",
        "#AC Alignment Check",
        "#MC Machine Check",
        "#XF SIMD Floating-Point Exception"
    };

    printf("Exception: %s\nError Code: 0x%x\n", err_desc[exp_nr], err_code);
}

// void divide_error(DWORD err_nr, DWORD err_code);
// void debug(DWORD err_nr, DWORD err_code);
// void nmi(DWORD err_nr, DWORD err_code);
// void breakpoint(DWORD err_nr, DWORD err_code);
// void overflow(DWORD err_nr, DWORD err_code);
// void bounds(DWORD err_nr, DWORD err_code);
// void invalid_op(DWORD err_nr, DWORD err_code);
// void device_not_available(DWORD err_nr, DWORD err_code);
// void double_fault(DWORD err_nr, DWORD err_code);
// void coprocessor_segment_overrun(DWORD err_nr, DWORD err_code);
// void invalid_TSS(DWORD err_nr, DWORD err_code);
// void segment_not_present(DWORD err_nr, DWORD err_code);
// void stack_segment(DWORD err_nr, DWORD err_code);
// void general_protection(DWORD err_nr, DWORD err_code);
// void page_fault(DWORD err_nr, DWORD err_code);
// void reserved(DWORD err_nr, DWORD err_code);
// void coprocessor_error(DWORD err_nr, DWORD err_code);
