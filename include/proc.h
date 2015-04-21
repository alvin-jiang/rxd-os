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

// runtime status
struct rts_struct {
    uint32 gs;
    uint32 fs;
    uint32 es;
    uint32 ds;
    uint32 edi;
    uint32 esi;
    uint32 ebp;
    uint32 _reserved;
    uint32 ebx;
    uint32 edx;
    uint32 ecx;
    uint32 eax;
    uint32 retaddr;
    uint32 ecode;
    uint32 eip;
    uint32 cs;
    uint32 eflags;
    uint32 esp;
    uint32 ss;
};

struct task_struct {
    int pid;
    struct task_struct *next;
    struct rts_struct rts;
    struct desc_struct ldt[2];
    int state;
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
    { 0x1b, 0x0f, 0x0f, 0x0f, \
        0, 0, 0, 0, 0, 0, 0, 0, \
/* retaddr, ecode */  0, 0, \
        (uint32)&init, 0x07, 0x3200, (uint32)&(init_task.ustack) + PAGE_SIZE, 0x0f}, \
    { \
        {0x9f, 0xc0fa00}, \
        {0x9f, 0xc0f200}, \
    }, TASK_RUNNING}

// not init ldt base & next task pointer
// #define NEW_TASK(pid, func) { \
//     pid, \
//     (void *)0, \
//     { 0x1b, 0x0f, 0x0f, 0x0f, \
//         0, 0, 0, 0, 0, 0, 0, 0, \
//         0, (uint32)&func, 0x07, 0x3200, PAGE_SIZE, 0x0f}, \
//     { \
//         {0x1, 0xc0fa00}, \
//         {0x1, 0xc0f200}, \
//     }}


extern struct task_struct *current_task;
extern void exception_handler(uint32 exp_nr, uint32 err_code, uint32 eip, uint32 cs, uint32 eflags);

void set_ldt_desc (struct task_struct *p_task);

void sched_init();
// void trap_init();

void init();

extern void sleep_on(struct task_struct ** p);
extern void wake_up(struct task_struct ** p);
extern int fork (void);

#endif
