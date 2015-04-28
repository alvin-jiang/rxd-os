/**
 *
 * @file: main.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-26
 *
 */

#include "mm.h"
#include "io.h"
#include "proc.h"

void print_boot_params ()
{
    printf("Cursor pos: %d\n", BOOT_PARAM_CURSOR);
    printf("Ex memory size: 0x%x\n", BOOT_PARAM_MEM_SIZE);
    printf("HD sectors-per-track: %d\n", BOOT_PARAM_HD_SECT_PER_TRK);
    printf("HD head count: %d\n\n", BOOT_PARAM_HD_HEAD_NR);
}

void main(void)
{
    printk("\n\n");
    printk("hello, RXD-OS!\n");
    print_boot_params();
    mem_init(0x100000, 0xffffff);

    // hd_init();
    // io_init();
    
    sched_init();
    move_to_user_mode();
    
    while(1);
}

