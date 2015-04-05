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
    // int *_16MB = (int *)0x1000000;
    // *_16MB = 1;
    
    sched_init();
    back_to_user_mode();
    
    while(1);
}

