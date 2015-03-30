#include "head.h"
#include "proc.h"

extern struct desc_struct *_gdt;
extern struct gate_struct *_idt;

struct desc_struct * gdt = (struct desc_struct *)&_gdt;
struct gate_struct * idt = (struct gate_struct *)&_idt;
struct tss_struct tss;

union task_union {
    struct task_struct task;
    char stack[4096];
};

static union task_union init_task = {INIT_TASK};
struct task_struct *current_task = &init_task.task;

static inline void set_base(struct desc_struct *p_desc, DWORD base)
{
    p_desc->low &= 0x00ff;
    p_desc->low |= base << 16;
    p_desc->high &= 0x00ffff00;
    p_desc->high |= (base >> 16 & 0xff) | (base & 0xff000000);
}
static inline void set_limit(struct desc_struct *p_desc, DWORD limit)
{
    limit &= 0xfffff;
    p_desc->low = (p_desc->low & 0xffff0000) | (limit & 0x0000ffff);
    p_desc->high = (p_desc->high & 0xfff0ffff) | (limit & 0xf0000);
}

void set_tss_desc(struct task_struct *p_task)
{
    // assert((p_task->rts.ss & 0xffff0000) == 0)
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = p_task->rts.ss;
    tss.esp0 = (DWORD)&p_task->rts + sizeof(struct rts_struct); // end of rts
    // TODO: what iobase means?
    tss.iobase = sizeof(tss);
    set_base(&gdt[4], (DWORD)&tss);
}

void set_ldt_desc(struct task_struct *p_task)
{
    printf("pid = %d\n", p_task->pid);
    printf("current = %x\n", current_task);

    struct desc_struct ldt_desc = INIT_IDT_DESC;
    set_base(&ldt_desc, (DWORD)&p_task->ldt);
    set_limit(&ldt_desc, (DWORD)(sizeof(struct desc_struct) * 2));

    memcpy(&gdt[5 + p_task->pid], &ldt_desc, sizeof(struct desc_struct));
    printf("ldt = %x %x\n", gdt[5 + p_task->pid].high, gdt[5 + p_task->pid].low);
}

void proc_init()
{
    set_tss_desc(&init_task.task);
    set_ldt_desc(&init_task.task);
}

void init()
{
    printf("hello, proc init()!\n");
    while(1);
}

