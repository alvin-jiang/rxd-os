
#include "head.h"
#include "proc.h"
#include "mm.h"

#define NR_SYSCALL  128
#define _64MB       0x4000000

void do_fork ();

syscall syscall_table[NR_SYSCALL] = {
    do_fork,
};

extern uint32 * ret_from_fork;

void do_fork ()
{
    printk("hello, do_fork()!\n");
    // get pid

    /* copy task */
    // new page, copy task_struct & kernel stack
    struct task_struct * p_task = (struct task_struct *)get_phy_page();
    memcpy((void *)p_task, (void *)current_task, PAGE_SIZE);
    p_task->pid = 1;
    // setup ldt
    set_desc_base(&(p_task->thread.ldt[0]), (uint32)(p_task->pid * _64MB));
    set_desc_base(&(p_task->thread.ldt[1]), (uint32)(p_task->pid * _64MB));
    /* TODO: set desc limit */
    set_ldt_desc(p_task);
    // share VMA, set write-protect
    // share_vma(old_base, new_base, old_limit);
    share_vma((uint32)(current_task->pid * _64MB),
        (uint32)(p_task->pid * _64MB), 640 * 1024/* limit */);

    // kernel-space regs
    p_task->thread.eip = (uint32)&ret_from_fork;
    p_task->thread.esp = (uint32)(p_task) + PAGE_SIZE - 68;
    printf("p_task: %x eip: %x esp: %x\n",
        p_task, p_task->thread.eip, p_task->thread.esp);

    // user-space regs
    *((uint32 *)current_task + (PAGE_SIZE-24) / 4) = p_task->pid;
    *((uint32 *)p_task + (PAGE_SIZE-24) / 4) = 0;
    // printf("%x %x\n", ((uint32 *)current_task + (PAGE_SIZE-20) / 4),
    //     *((uint32 *)current_task + (PAGE_SIZE-20) / 4));

    // update task list
    p_task->next = current_task->next;
    current_task->next = p_task;

    // switch to child task
    switch_to(current_task, p_task, current_task);
}


