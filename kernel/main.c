/**
 *
 * @file: main.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-26
 *
 */

#include "proc.h"

void main(void)
{
    printk("\n\n");
    printk("hello RXD-OS! I'm printk !!!\n");
    //mem_init(0x100000, BOOT_PARAM_MEM_SIZE);
    proc_init();

    move_to_user_mode();

    // if (!fork()) { // copy, set eip, tss, ldt
    //     init();
    // }

    // loop forever
    while(1);
}

