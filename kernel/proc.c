#include "head.h"
#include "proc.h"

union task_union {
    struct task_struct task;
    char stack[4096];
};

static union task_union init_task = {INIT_TASK};
struct task_struct *current_task = &init_task.task;

static inline void set_base(struct desc_struct *p_desc, DWORD base)
{
    p_desc->low &= 0x00ff;
    p_desc->low |= base << 16;
    p_desc->high &= 0x00ffff00;
    p_desc->high |= (base >> 16 & 0xff) | (base & 0xff000000);
}
static inline void set_limit(struct desc_struct *p_desc, DWORD limit)
{
    limit &= 0xfffff;
    p_desc->low = (p_desc->low & 0xffff0000) | (limit & 0x0000ffff);
    p_desc->high = (p_desc->high & 0xfff0ffff) | (limit & 0xf0000);
}

void set_ldt_desc(struct task_struct *p_task)
{
    printf("pid = %d\n", p_task->pid);
    printf("current = %x\n", current_task);

    struct desc_struct ldt_desc = INIT_IDT_DESC;
    set_base(&ldt_desc, (DWORD)&p_task->ldt);
    set_limit(&ldt_desc, (DWORD)(sizeof(struct desc_struct) * 2));

    memcpy(&gdt[5 + p_task->pid], &ldt_desc, sizeof(struct desc_struct));
    printf("ldt = %x %x\n", gdt[5 + p_task->pid].high, gdt[5 + p_task->pid].low);
}

void proc_init()
{
    // set_tss_desc(&init_task.task);
    set_ldt_desc(&init_task.task);

    // set timer_interrupt
}

void init()
{
    printf("hello, proc init()!\n");
    while(1);
}

// void schedule()
// {
//     if( current_task->next != current_task )
//         current_task = current_task->next;
// }

// void clock_interrupt(int int_nr)
// {
//     schedule();
// }

// void clock_init()
// {
//     // init 8253 PIT
//     // out_byte(TIMER_MODE, RATE_GENERATOR);
//     // out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
//     // out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

//     // set_int_handler(INT_REQ_CLOCK, clock_interrupt);
//     // enable_int(INT_REQ_CLOCK);
// }

