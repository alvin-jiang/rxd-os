
#include "head.h"
#include "entry.h"
#include "proc.h"
#include "mm.h"
#include "fs.h"
#include "stdio.h"

#define NR_SYSCALL  128
#define _64MB       0x4000000

void do_fork (void);

syscall syscall_table[NR_SYSCALL] = {
    do_fork,
    /*sys_open,*/
    /*sys_close,*/
    /*sys_read,*/
    /*sys_write,*/
};

extern u32 * ret_from_fork;

void do_fork (void)
{
    /* get pid */
    int child_pid;
    if ((child_pid = get_pid()) < 0) {
        RETURN(-1);
    }

    /* Task Struct */
    struct task_struct * child = (struct task_struct *)alloc_page();
    copy_process(child, current_task);
    child->pid = child_pid;

    /* Memory */
    // allocate virtual memory for child process
    set_desc_base(&(child->thread.ldt[0]), (u32)(child->pid * _64MB));
    set_desc_base(&(child->thread.ldt[1]), (u32)(child->pid * _64MB));
    /* TODO: set desc limit */
    set_ldt_desc(child);
    // share VMA, set write-protect
    // share_vma(old_base, new_base, old_limit);
    share_vma((addr_t)(current_task->pid * _64MB),
        (addr_t)(child->pid * _64MB), 640 * 1024/* limit */);

    /* File System */

    /* Resume Point & Return Value */
    child->thread.eip = (unsigned long)&ret_from_fork;
    child->thread.esp = (unsigned long)FORK_STACK(child);
    // parent process returns child pid
    SET_TASK_RETVAL(current_task, child->pid);
    // child process returns 0
    SET_TASK_RETVAL(child, 0);

    printf("parent: %x cs: %x eip: %x esp: %x\n",
        current_task, current_task->thread.eip, current_task->thread.esp);
    printf("child: %x cs: %x eip: %x esp: %x\n",
        child, child->thread.eip, child->thread.esp);

    /* add child to task table, so it can be scheduled */
    add_task(child);
    schedule();
}

