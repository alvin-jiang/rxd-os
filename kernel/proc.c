#include "proc.h"

#include "assert.h"

union task_union {
    struct task_struct task;
    char stack[PAGE_SIZE];
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

static union task_union task2;

void sched_init ()
{
    // set task 0
    set_ldt_desc(&init_task.task);
    // set test task 1
    // memcpy(&task2.task, &init_task.task, PAGE_SIZE);
    // task2.task.pid = 1;
    // task2.task.rts.eip = (DWORD)&init2;
    // task2.task.rts.esp = (DWORD)&task2 + PAGE_SIZE;
    // set_ldt_desc(&task2.task);
    // // link tasks
    // init_task.task.next = &(task2.task);
    // task2.task.next = &(init_task.task);

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
    printf("hello, proc init()!\n");
    while (1) {
        int i;
        for (i = 0; i < 100000; ++i)
            ;
        printf("_");
    }
}

void init2()
{
    while (1) {
        int i;
        for (i = 0; i < 100000; ++i)
            ;
        printf("#");
    }
}

void on_clock_interrupt (int int_nr)
{
    // while(1);
    printk("^");
    assert( current_task && current_task->next );
    current_task = current_task->next;
}

