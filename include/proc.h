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

struct thread_info {
    uint32 eip, esp;    /* used only in switch_to */
    struct desc_struct ldt[2];
};

struct task_struct {
    int pid;
    struct task_struct *next;
    struct thread_info thread;
};

#define TASK_RUNNING            0
// #define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
// #define TASK_ZOMBIE             3
// #define TASK_STOPPED            4

// base = 0, limit = 0x9f000
#define INIT_TASK { \
    0, \
    &init_task.task, \
    { (uint32)&init, (uint32)&(init_task.ustack) + PAGE_SIZE, \
        {{0x9f, 0xc0fa00}, {0x9f, 0xc0f200}}}}

extern struct task_struct *current_task;
extern void exception_handler(uint32 exp_nr, uint32 err_code, uint32 eip, uint32 cs, uint32 eflags);

void set_ldt_desc (struct task_struct *p_task);

void sched_init();

void init();

extern void sleep_on(struct task_struct ** p);
extern void wake_up(struct task_struct ** p);
extern int fork (void);

extern void dump_task (struct task_struct *p);

/*
    switch_to store non-volatile regs on stack,
    including esi, edi, eflags, ebp
*/
#define switch_to(prev,next,last) do { \
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
