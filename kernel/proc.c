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

void set_ldt_desc (struct task_struct *p_task)
{
    struct desc_struct ldt_desc = INIT_IDT_DESC;
    set_desc_base(&ldt_desc, (DWORD)&p_task->ldt);
    set_desc_limit(&ldt_desc, (DWORD)(sizeof(struct desc_struct) * 2));

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
    int i;
    int pid = fork();
    // printf("hello, fork pid = %x\n", pid);
    if (pid) {
        printf("hello, I'm child proc [%x]\n", pid);
    }
    else {
        // delay, so child proc can return correctly
        for (i = 0; i < 100000; ++i)
            ;
        printf("hello, I'm parent proc [%x]\n", pid);
    }
    while (1) {
        // int i;
        // for (i = 0; i < 100000; ++i)
        //     ;
        // printk("^");
    }
}

void on_clock_interrupt (int int_nr)
{
    if (int_reenter != 0)
        return;

    assert( current_task && current_task->next );
    current_task = current_task->next;
    // printk(".");
    printf("\r%x", current_task->pid);
}

