#include "proc.h"

#include "assert.h"

union task_union {
    struct task_struct task;
    struct {
        char kstack[PAGE_SIZE]; // kernel stack
        char ustack[PAGE_SIZE]; // user stack
    };
};
static union task_union init_task = {INIT_TASK};

struct task_struct *current_task = &init_task.task;

void on_clock_interrupt (int int_nr);

static inline void set_base (struct desc_struct *p_desc, DWORD base)
{
    p_desc->low &= 0x00ff;
    p_desc->low |= base << 16;
    p_desc->high &= 0x00ffff00;
    p_desc->high |= (base >> 16 & 0xff) | (base & 0xff000000);
}
static inline void set_limit (struct desc_struct *p_desc, DWORD limit)
{
    limit &= 0xfffff;
    p_desc->low = (p_desc->low & 0xffff0000) | (limit & 0x0000ffff);
    p_desc->high = (p_desc->high & 0xfff0ffff) | (limit & 0xf0000);
}

void set_ldt_desc (struct task_struct *p_task)
{
    struct desc_struct ldt_desc = INIT_IDT_DESC;
    set_base(&ldt_desc, (DWORD)&p_task->ldt);
    set_limit(&ldt_desc, (DWORD)(sizeof(struct desc_struct) * 2));

    memcpy(&gdt[GDT_IDX_FIRST_LDT + p_task->pid], &ldt_desc, sizeof(struct desc_struct));
}

void sched_init ()
{
    int_reenter = 0;
    // set task 0
    set_ldt_desc(&init_task.task);

    // set clock interrupt
    // NR_INT_CLOCK
    set_int_callback(0, on_clock_interrupt);
    enable_int(0);
    // init 8253 PIT
    // out_byte(TIMER_MODE, RATE_GENERATOR);
    // out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
    // out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));
}

void init ()
{
    // printk("hello, proc init()!\n");
    int *_16MB = (int *)0x1000000;
    int _i16MB = *_16MB;
    *_16MB = _i16MB;
    while (1) {
        // int i;
        // for (i = 0; i < 100000; ++i)
        //     ;
        // printk("^");
    }
}

void on_clock_interrupt (int int_nr)
{
    // while(1);
    printk("_");
    assert( current_task && current_task->next );
    current_task = current_task->next;
}

