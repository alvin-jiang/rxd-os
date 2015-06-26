#include "proc.h"
#include "mm.h"
#include "assert.h"
#include "string.h"

extern void init (void);

union task_union {
    struct task_struct task;
    struct {
        char kstack[PAGE_SIZE]; // kernel stack
        char ustack[PAGE_SIZE]; // user stack
    };
};
static union task_union init_task = {INIT_TASK};
static struct task_struct * task_table[NR_TASK];
struct task_struct *current_task;

void on_clock_interrupt (int int_nr);

void set_ldt_desc (struct task_struct *p_task)
{
    struct desc_struct ldt_desc = INIT_IDT_DESC;
    set_desc_base(&ldt_desc, (u32)&p_task->thread.ldt);
    set_desc_limit(&ldt_desc, (u32)(sizeof(struct desc_struct) * 2));

    memcpy(&gdt[GDT_IDX_FIRST_LDT + p_task->pid], &ldt_desc, sizeof(struct desc_struct));
}

int get_pid (void)
{
    int i;
    for (i = 0; i < NR_TASK; ++i) {
        if (task_table[i] == NULL)
            return i;
    }
    return -1;
}

void add_task(struct task_struct * task)
{
    assert(task_table[task->pid] == NULL);
    task_table[task->pid] = task;
}

/*
    make a complete copy of parent process,
    includs task struct, kernel stack (both lie in the same and only page).
*/
void copy_process(struct task_struct * child, struct task_struct * parent)
{
    memcpy((void *)child, (void *)parent, PAGE_SIZE);
}

void sched_init ()
{
    int i;
    // init task table
    for (i = 0; i < NR_TASK; ++i) {
        task_table[i] = NULL;
    }
    task_table[0] = &init_task.task;
    current_task = task_table[0];

    // init task 0
    set_ldt_desc(&init_task.task);

    // set clock interrupt
    // NR_INT_CLOCK
    set_irq_handler(IRQ0_CLOCK, on_clock_interrupt);
    enable_irq(IRQ0_CLOCK);
    // init 8253 PIT
    // outb(TIMER_MODE, RATE_GENERATOR);
    // outb(TIMER0, (u8) (TIMER_FREQ/HZ) );
    // outb(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));
}

void FASTCALL __switch_to(struct task_struct * prev, struct task_struct * next)
{
    /* switch kernel stack */
    tss->esp0 = (unsigned long)(next) + PAGE_SIZE;
    /* TODO: this should be in memory switch function */
    lldt(next->pid);

    /* switch task */
    current_task = next;
}

void schedule (void)
{
    struct task_struct ** const START = task_table;
    struct task_struct ** const END = START + NR_TASK;
    /* find current task in task table */
    struct task_struct **p_table;
    for (p_table = START; p_table != END; ++p_table) {
        if (*p_table != NULL && *p_table == current_task)
            break;
    }
    assert(p_table != END);

    /* pick next task */
    ++p_table;
    if (p_table == END)
        p_table = START;
    do {
        if (*p_table != NULL && (*p_table)->timespan > 0)
            break;
        ++p_table;
        if (p_table == END)
            p_table = START;
    } while (*p_table != current_task);
    assert(*p_table != NULL);

    if ((*p_table)->timespan > 0) {
        (*p_table)->timespan -= 1;
        if (*p_table != current_task) {
            struct task_struct *p_task = *p_table;
            switch_to(current_task, p_task, current_task);
        }
    }
    else {
        /* no task has time span, reset all */
        for (p_table = START; p_table != END; ++p_table) {
            if (*p_table != NULL)
                (*p_table)->timespan = (*p_table)->priority;
        }
        schedule();
    }
}

void on_clock_interrupt (int int_nr)
{
    schedule();
}

void sleep_on(struct task_struct ** p)
{
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
    u32 * kstack = (u32 *)p + PAGE_SIZE / sizeof(u32 *);
    int i;

    printf("kernel stack: %x\n", kstack);
    for (i = 0; i < 17; ++i) {
        printf("%s: %x ", regs[i], *(kstack - i - 1));
        if (!((i + 1) % 5))
            printf("\n");
    }
    printf("\n");
}
