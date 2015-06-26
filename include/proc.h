/**
 *
 * @file: proc.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-28
 *
 */

#ifndef __PROC_H__
#define __PROC_H__

#include "head.h"
#include "fs.h"

struct thread_info {
    unsigned long eip, esp;    /* used only in switch_to */
    struct desc_struct ldt[2];
};

#define NR_MAX_OPEN 10

struct task_struct {
    /* touch this should change entry.h & entry.asm */
    int pid;
    struct thread_info thread;

    /* add new members here */
    int priority;
    int timespan;
    /* fs */
    struct file * filp[NR_MAX_OPEN];
};

#define NR_TASK 64

#define TASK_RUNNING            0
// #define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
// #define TASK_ZOMBIE             3
// #define TASK_STOPPED            4

// base = 0, limit = 0xfff000
#define INIT_TASK { \
    0, \
    { (u32)&init, (u32)&(init_task.ustack) + PAGE_SIZE, \
        {{0xfff, 0xc0fa00}, {0xfff, 0xc0f200}} \
    }, \
    10, \
    10, \
/* other */ }

extern struct task_struct *current_task;

void set_ldt_desc (struct task_struct *p_task);

void sched_init();

extern int get_pid (void);
extern void add_task(struct task_struct * task);
extern void copy_process(struct task_struct * child, struct task_struct * parent);

extern void schedule(void);
extern void sleep_on(struct task_struct ** p);
extern void wake_up(struct task_struct ** p);
extern void dump_task (struct task_struct *p);

/*
    switch_to stores non-volatile regs on stack,
    including esi, edi, eflags, ebp
*/
#define switch_to(prev,next,last) do { \
    if ((prev) == (next)) \
        break; \
    unsigned long esi,edi; \
    asm volatile("pushfl\n\t" \
             "pushl %%ebp\n\t" \
             "movl %%esp,%0\n\t"    /* save ESP */      \
             "movl %5,%%esp\n\t"    /* restore ESP */   \
             "movl $1f,%1\n\t"      /* save EIP */      \
             "pushl %6\n\t"         /* restore EIP */   \
             "jmp __switch_to\n" \
             "1:\t" \
             "popl %%ebp\n\t" \
             "popfl" \
             :"=m" (prev->thread.esp),"=m" (prev->thread.eip), \
              "=a" (last),"=S" (esi),"=D" (edi) \
             :"m" (next->thread.esp),"m" (next->thread.eip), \
              "c" (prev), "d" (next)); \
} while (0)

extern void __switch_to(struct task_struct * prev, struct task_struct * next) FASTCALL;

#endif
