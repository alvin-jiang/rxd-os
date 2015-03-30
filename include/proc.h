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
    DWORD gs;
    DWORD fs;
    DWORD es;
    DWORD ds;
    DWORD edi;
    DWORD esi;
    DWORD ebp;
    DWORD kernel_esp;
    DWORD ebx;
    DWORD edx;
    DWORD ecx;
    DWORD eax;
    DWORD retaddr;
    DWORD eip;
    DWORD cs;
    DWORD eflags;
    DWORD esp;
    DWORD ss;
};

struct task_struct {

    // tty
    // file desc
    // priority
    // name
    // char pname[128];

    int pid;
    struct rts_struct rts;
    struct desc_struct ldt[2]; // code, data
};

// base = 0, limit = 0x9f000
#define INIT_TASK { \
    0, \
    { 0x1b, 0x0f, 0x0f, 0x0f, \
        0, 0, 0, 0, 0, 0, 0, 0, \
        0, (DWORD)&init, 0x07, 0, (DWORD)&init_task + 4096, 0x0f}, \
    { \
        {0x9f, 0xc0fa00}, \
        {0x9f, 0xc0f200}, \
    }}
#define INIT_IDT_DESC {0x0, 0x00408200}

extern struct task_struct *current_task;

void proc_init();
void init();

#endif
