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
#include "blk.h"
#include "proc.h"
#include "syscall.h"
// #include "fs.h"
#include "string.h"
#include "entry.h"

void print_boot_params ()
{
    printf("Cursor pos: %d\n", BOOT_PARAM_CURSOR);
    printf("Ex memory size: 0x%x\n", BOOT_PARAM_MEM_SIZE);
    printf("HD sectors-per-track: %d\n", BOOT_PARAM_HD_SECT_PER_TRK);
    printf("HD head count: %d\n\n", BOOT_PARAM_HD_HEAD_NR);
}

void kmain(void)
{
    printk("\n\n");
    printk("hello, RXD-OS!\n");
    print_boot_params();

    mem_init(0x100000, 0xffffff);
    cache_init();

    blk_dev_init();
    /* 128K ~ 576K, 448K */
    buffer_init(0x20000, 0x90000);
    io_init();
    fs_init();
    
    sched_init();
    move_to_user_mode();
    
    while(1);
}

_syscall0(int, fork);
/* open(pathname, flags, ...) */
_syscall2(int, open, const char *, pathname, int, flags);
_syscall1(int, close, int, fd);
_syscall3(ssize_t, read, int, fd, void *, buf, size_t, count);
_syscall3(ssize_t, write, int, fd, const void *, buf, size_t, count);
_syscall3(off_t, lseek, int, fd, off_t, offset, int, whence);

void mm_test(void);
void cache_test(void);
void buffer_test(void);
void fork_test(void);

void init ()
{
    printf("hello, init()\n");

    // mm_test();
    // cache_test();

    // int fd = open("/abc", O_CREAT);
    // printf("open() returns %d\n", fd);
    // fs_dump_cache();
    // if (fd != -1) {
    //     close(fd);
    //     fs_dump_cache();
    // }

    int ret;
    char buffer[256];
    const char *str = "hello, filesystem!";
    int fd = open("/abc", O_CREAT);
    printf("open() returns %d\n", fd);
    if (fd != -1) {
        ret = lseek(fd, 0, SEEK_END);
        printf("lseek() returns %d\n", ret);
        ret = write(fd, str, strlen(str) + 1);
        assert(ret == strlen(str) + 1);
        // ret = read(fd, buffer, sizeof(buffer));
        // assert(ret > 0);
        // buffer[ret] = '\0';
        printf("from file-system: %s\n", buffer);
        close(fd);
    }

    while (1);
}

void mm_test(void)
{
    unsigned long page;
    unsigned long pages[10];
    page = alloc_page();
    free_page(page);
    page = alloc_page();
    free_page(page);

    page = alloc_pages(3);
    pages[0] = alloc_page();
    free_pages(page, 3);
    free_page(pages[0]);

    page = alloc_page();
    free_page(page);
}

void cache_test(void)
{
    printf("\ncache test:\n");
    int *ip = kmalloc(4);
    kfree(ip);
    ip = kmalloc(4);
    kfree(ip);

    void * objs[8];
    int i;
    for (i = 0; i < 8; ++i) {
        objs[i] = kmalloc(i + 1);
    }
    for (i = 0; i < 8; ++i) {
        kfree(objs[i]);
    }

    printf("test end\n");
}

void buffer_test(void)
{
    struct buffer_head *bh;
    int i;
    for (i = 0; i < 4; ++i) {
        if ((bh = bread(3, i % 2)) != NULL) {
            // printf("Read Sector %d:\n", i);
            // for (k = 0; k < 30; ++k)
            //     printf("%x ", bh->b_data[k]);
            // printf("\n");
            strcpy(bh->b_data + 5, "hello world!");
            bwrite(bh);
            brelease(bh);
            buffer_stat();
            printf("\n");
        }
    }
}

void fork_test(void)
{
    int i;
    int pid;
    if (!(pid = fork())) {
        printf("hello, I'm child proc\n");
    }
    else {
        printf("hello, I'm parent proc, child [%x]\n", pid);
    }
    for (i = 0; i < 5; ++i) {
        if (!(pid = fork())) {
            printf("hello, I'm child proc\n");
        }
        else {
            printf("hello, I'm parent proc, forked [%x]\n", pid);
        }
    }
}