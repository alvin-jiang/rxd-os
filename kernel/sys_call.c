
#include "head.h"
#include "proc.h"
#include "mm.h"

#define NR_SYSCALL  128
#define _64MB       0x4000000

void do_fork ();

syscall syscall_table[NR_SYSCALL] = {
    do_fork,
};

void do_fork ()
{
    printk("hello, do_fork()!\n");
    // get pid

    // new page, copy task_struct
    struct task_struct * p_task = (struct task_struct *)get_phy_page();
    memcpy((void *)p_task, (void *)current_task, PAGE_SIZE);
    p_task->pid = 1;

    // set ldt
    set_desc_base(&(p_task->ldt[0]), (uint32)(p_task->pid * _64MB));
    set_desc_base(&(p_task->ldt[1]), (uint32)(p_task->pid * _64MB));
    set_ldt_desc(p_task);
    // share VMA, set write-protect
    // share_vma(old_base, new_base, old_limit);
    share_vma((uint32)(current_task->pid * _64MB),
        (uint32)(p_task->pid * _64MB), 640 * 1024/* limit */);

    // set return values
    p_task->rts.eax = p_task->pid;
    current_task->rts.eax = 0;
    printf("eax: %x  child_eax: %x eip: %x chile_eip: %x\n",
        current_task->rts.eax, p_task->rts.eax,
        current_task->rts.eip, p_task->rts.eip);

    // update task list
    p_task->next = current_task->next;
    current_task->next = p_task;
}

