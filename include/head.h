
#ifndef __HEAD_H__
#define __HEAD_H__

#include "type.h"

struct desc_struct
{
    DWORD low, high;
};

struct gate_struct
{
    WORD    offset_low;
    WORD    selector;
    BYTE    __reserved;   // we don't use call gate
    BYTE    attr;
    WORD    offset_high;
};

struct tss_struct
{
    DWORD __reserved0;
    DWORD esp0;
    DWORD ss0;
    WORD __reserved1[45];
    WORD iobase;
};

extern struct desc_struct *gdt;
extern struct gate_struct *idt;
extern struct tss_struct tss;

// TODO: make this asm macro
void set_base(struct desc_struct *p_desc, DWORD base);
void set_limit(struct desc_struct *p_desc, DWORD limit);
// #define set_base(addr, base) ()
// #define set_limit(addr, limit) ()

// boot params
#define BOOT_PARAM_CURSOR (*(unsigned short *)0x90000)
#define BOOT_PARAM_MEM_SIZE (*(unsigned long *)0x90002)

// #define GDT_SEL_NULL        (0x00)
// #define GDT_SEL_CODE        (0x08)
// #define GDT_SEL_DATA        (0x10)
// #define GDT_SEL_VIDEO       (0x18+3)
// #define GDT_SEL_TSS         (0x20)
// #define GDT_SEL_LDT_FIRST   (0x28)
// #define GDT_SEL_LDT_LAST    (0xfff8)

// #define LDT_CODE_ATTR_DEFAULT (0x9A | 0x4000 | 0x8000 | 0x60)
// #define LDT_DATA_ATTR_DEFAULT (0x92 | 0x4000 | 0x8000 | 0x60)

// #define PRIVILEGE_KRNL  0
// #define PRIVILEGE_TASK  1
// #define PRIVILEGE_USER  3
// #define RPL_KRNL    SA_RPL0
// #define RPL_TASK    SA_RPL1
// #define RPL_USER    SA_RPL3

// #define SELECTOR_KERNEL_CS  SELECTOR_FLAT_C
// #define SELECTOR_KERNEL_DS  SELECTOR_FLAT_RW
// #define SELECTOR_KERNEL_GS  SELECTOR_VIDEO

// /* 每个任务有一个单独的 LDT, 每个 LDT 中的描述符个数: */
// #define LDT_SIZE        2

// /* 描述符类型值说明 */
// #define DA_32           0x4000  /* 32 位段                */
// #define DA_LIMIT_4K     0x8000  /* 段界限粒度为 4K 字节         */
// #define DA_DPL0         0x00    /* DPL = 0              */
// #define DA_DPL1         0x20    /* DPL = 1              */
// #define DA_DPL2         0x40    /* DPL = 2              */
// #define DA_DPL3         0x60    /* DPL = 3              */
// /* 存储段描述符类型值说明 */
// #define DA_DR           0x90    /* 存在的只读数据段类型值      */
// #define DA_DRW          0x92    /* 存在的可读写数据段属性值     */
// #define DA_DRWA         0x93    /* 存在的已访问可读写数据段类型值  */
// #define DA_C            0x98    /* 存在的只执行代码段属性值     */
// #define DA_CR           0x9A    /* 存在的可执行可读代码段属性值       */
// #define DA_CCO          0x9C    /* 存在的只执行一致代码段属性值       */
// #define DA_CCOR         0x9E    /* 存在的可执行可读一致代码段属性值 */
// /* 系统段描述符类型值说明 */
// #define DA_LDT          0x82    /* 局部描述符表段类型值           */
// #define DA_TaskGate     0x85    /* 任务门类型值               */
// #define DA_386TSS       0x89    /* 可用 386 任务状态段类型值      */
// #define DA_386CGate     0x8C    /* 386 调用门类型值           */
// #define DA_386IGate     0x8E    /* 386 中断门类型值           */
// #define DA_386TGate     0x8F    /* 386 陷阱门类型值           */

// /* 选择子类型值说明 */
// /* 其中, SA_ : Selector Attribute */
// #define SA_RPL_MASK 0xFFFC
// #define SA_RPL0     0
// #define SA_RPL1     1
// #define SA_RPL2     2
// #define SA_RPL3     3

// #define SA_TI_MASK  0xFFFB
// #define SA_TIG      0
// #define SA_TIL      4

// /* 中断向量 */
// #define INT_VECTOR_DIVIDE       0x0
// #define INT_VECTOR_DEBUG        0x1
// #define INT_VECTOR_NMI          0x2
// #define INT_VECTOR_BREAKPOINT       0x3
// #define INT_VECTOR_OVERFLOW     0x4
// #define INT_VECTOR_BOUNDS       0x5
// #define INT_VECTOR_INVAL_OP     0x6
// #define INT_VECTOR_COPROC_NOT       0x7
// #define INT_VECTOR_DOUBLE_FAULT     0x8
// #define INT_VECTOR_COPROC_SEG       0x9
// #define INT_VECTOR_INVAL_TSS        0xA
// #define INT_VECTOR_SEG_NOT      0xB
// #define INT_VECTOR_STACK_FAULT      0xC
// #define INT_VECTOR_PROTECTION       0xD
// #define INT_VECTOR_PAGE_FAULT       0xE
// #define INT_VECTOR_COPROC_ERR       0x10

// /* 中断向量 */
// #define INT_VECTOR_IRQ0         0x20
// #define INT_VECTOR_IRQ8         0x28

// /* 系统调用 */
// #define INT_VECTOR_SYS_CALL             0x90

#endif
