#include "proc.h"

#include "hd.h"
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
    set_desc_base(&ldt_desc, (uint32)&p_task->thread.ldt);
    set_desc_limit(&ldt_desc, (uint32)(sizeof(struct desc_struct) * 2));

    memcpy(&gdt[GDT_IDX_FIRST_LDT + p_task->pid], &ldt_desc, sizeof(struct desc_struct));
}

void sched_init ()
{
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
    if (!pid) {
        printf("hello, I'm child proc\n");
    }
    else {
        // delay, so child proc can return correctly
        for (i = 0; i < 100000; ++i)
            ;
        printf("hello, I'm parent proc, child [%x]\n", pid);
        // hd_read(0, 0, 0);
    }
    while (1) {
        // int i;
        // for (i = 0; i < 100000; ++i)
        //     ;
        // if (!pid)
        //     printf("c");
        // else
        //     printf("p");
    }
}

void FASTCALL __switch_to(struct task_struct * prev, struct task_struct * next)
{
    // TSS, LDT
    tss->esp0 = (uint32)(next) + PAGE_SIZE;
    // load LDT
    lldt(next->pid);

    current_task = next;
}

void schedule (void)
{
    assert( current_task && current_task->next );
    // do {
    // printk("^");
        // current_task = current_task->next;
    // } while (current_task->state != TASK_RUNNING);

    switch_to(current_task, current_task->next, current_task);
}

void on_clock_interrupt (int int_nr)
{
    schedule();
}

void sleep_on(struct task_struct ** p)
{
    // struct task_struct *tmp;

    // if (!p)
    //     return;
    // if (current_task == &(init_task.task))
    //     panic("task[0] trying to sleep");
    // tmp = *p;
    // *p = current_task;
    // current_task->state = TASK_UNINTERRUPTIBLE;
    // schedule();
    // if (tmp)
    //     tmp->state=0;
}
void wake_up(struct task_struct ** p)
{

}

void dump_task (struct task_struct *p)
{
    const char * regs[] = {
        "ss", "esp", "eflags", "cs", "eip",
        "eax", "ecx", "edx", "ebx", "",
        "ebp", "esi", "edi",
        "ds", "es", "fs", "gs"
    };
    uint32 * kstack = (uint32 *)p + PAGE_SIZE / sizeof(uint32 *);
    int i;

    printf("kernel stack: %x\n", kstack);
    for (i = 0; i < 17; ++i) {
        printf("%s: %x ", regs[i], *(kstack - i - 1));
        if (!((i + 1) % 5))
            printf("\n");
    }
    printf("\n");
}
