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
    io_init();

    hd_init();
    char buffer[512 * 3];
    hd_rdwt (0, 0, 3, buffer);
    buffer[0] = 1;
    buffer[512] = 2;
    buffer[1024] = 3;
    hd_rdwt (1, 0, 3, buffer);
    printf("After write:\n");
    hd_rdwt (0, 0, 3, buffer);
    
    sched_init();
    back_to_user_mode();
    
    while(1);
}

