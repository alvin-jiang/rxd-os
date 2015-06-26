/*
    We only support primary IDE channel now.
*/

#include "blk.h"

#include "head.h"
#include "stdio.h"
#include "assert.h"

// write
#define DATA        0x1f0
#define FEATURES    0x1f1
#define SECT_CNT    0x1f2
#define ADDR_0      0x1f3
#define ADDR_1      0x1f4
#define ADDR_2      0x1f5

#define DEVICE      0x1f6
#define DEV_DISK_0  0x00
// #define DEV_DISK_1  0x10
#define DEV_CHS     0xa0
#define DEV_LBA28   0xe0
#define DEV_LBA48   0x40

#define COMMAND         0x1f7
#define CMD_IDENTIFY    0xec
#define CMD_READ        0x20 /* Read Sector(s) */
#define CMD_WRITE       0x30 /* Write Sector(s) */
#define CMD_RW48        0x04 /* Read/Write in LBA48 mode */

#define DEV_CTRL        0x3f6
#define DCTL_RESET      0x04
#define DCTL_INT_ON     0x00 /* Enable Interrupt */
#define DCTL_INT_OFF    0x02 /* Disable Interrupt */

// read
#define ERROR       FEATURES

#define STATUS      COMMAND
#define STAT_ERR    0x01
// #define STAT_INDEX  0x02 /* obsolete */
// #define STAT_ECC    0x04 /* obsolete */
#define STAT_DRQ    0x08 /* Data Request, ready to transfer data */
#define STAT_SEEK   0x10
#define STAT_WRERR  0x20 /* Device Fault/Stream Error */
#define STAT_READY  0x40 /* Drive Ready */
#define STAT_BUSY   0x80

#define ALT_STATUS  DEV_CTRL

#define SECTOR_SIZE 512
#define READ    0
#define WRITE   1
// typedef struct hd_partition {
//     u8    boot_ind;
//     u8    start_head;
//     u8    start_sector;
//     u8    start_cyl;
//     u8    sys_id;
//     u8    end_head; 
//     u8    end_sector;
//     u8    end_cyl;
//     u32   start_sect; 
//     u32   nr_sects;
// } hd_partition;

typedef struct hd_dev
{
    int     addr_mode; /* 0:C/H/S 1:LBA28 2:LBA48 */
    u32     sect_cnt;
    u32     sect_cnt_ex;
} hd_dev;

static hd_dev hd[2] = {{0, 0, 0}, {0, 0, 0}};

#define NORM_WAIT_TIME 10000
#define LONG_WAIT_TIME 100000

static int wait_status (int stat, int value, int time_out)
{
    assert( time_out > 0 );

    while (--time_out && (inb_p(STATUS) & stat) != value);
    return time_out;
}

/* retrieve hard disk information */
static void hd_identity (u8 dev_nr)
{
    assert( dev_nr <= 1 );

    outb(DEV_CTRL, DCTL_INT_OFF);
    outb(SECT_CNT, 0);
    outb(ADDR_0,  0);
    outb(ADDR_1, 0);
    outb(ADDR_2, 0);
    outb(DEVICE, (dev_nr << 4) | DEV_CHS);
    outb(COMMAND, CMD_IDENTIFY);
    if (!inb(STATUS)) {
        printf("HD: no disk %d\n", dev_nr);
        return;
    }
    if (!wait_status(STAT_BUSY | STAT_DRQ, STAT_DRQ, LONG_WAIT_TIME)) {
        panic("HD: disk identify failed.");
    }

    /* read hard disk info */
    u16 buf[256];
    port_read(DATA, buf, sizeof(buf));
    hd[dev_nr].addr_mode = (buf[49] & 0x0200) ? ((buf[83] & 0x400) ? 2 : 1) : 0;
    if (hd[dev_nr].addr_mode == 1)
        hd[dev_nr].sect_cnt = (u32)(buf[61] << 16) | buf[60];
    else if (hd[dev_nr].addr_mode == 2) {
        hd[dev_nr].sect_cnt = (u32)(buf[101] << 16) | buf[100];
        hd[dev_nr].sect_cnt_ex = (u32)(buf[103] << 16) | buf[102];
    }

    /* print identified hard disk info */
    printf("HD: disk %d, %s, %d MB", dev_nr,
        hd[dev_nr].addr_mode ? ((hd[dev_nr].addr_mode == 2) ? "LBA48" : "LBA28") : "CHS",
        hd[dev_nr].sect_cnt >> 11);
    if (hd[dev_nr].sect_cnt_ex)
        printf(" + %d TB\n", hd[dev_nr].sect_cnt_ex << 2);
    else
        printf("\n");
}

/* send read/write command to disk drive */
static void hd_cmd_out (u8 dev_nr, u8 cmd,/* u8 features, */
    u32 sect_cnt, u32 nsect_low, u32 nsect_high)
{
    assert( dev_nr <= 1 );

    u32 a0, a1;
    u8 dev_reg;

    // miss limit check
    if (!sect_cnt || sect_cnt > (hd[dev_nr].addr_mode == 2 ? 65536 : 256))
        panic("HD: invalid sector count.");
    if (nsect_high != 0 || (nsect_low >= 0x10000000) )
        panic("HD: sorry, the largest capacity we support now is 128GB.");
    if (!wait_status(STAT_BUSY, 0, NORM_WAIT_TIME))
        panic("HD: controller not ready.");

    dev_reg = dev_nr << 4;
    switch (hd[dev_nr].addr_mode) {
    case 0: /* C/H/S */
        dev_reg |= DEV_CHS;
        /*
            LBA -> C/H/S
            Sector      = LBA % SectPerTrk + 1
            Cylinder    = (LBA / SectPerTrk) / HeadCnt
            Head        = (LBA / SectPerTrk) % HeadCnt
        */
        a0 = (nsect_low % BOOT_PARAM_HD_SECT_PER_TRK + 1) & 0x000000ff;
        a1 = nsect_low / BOOT_PARAM_HD_SECT_PER_TRK;
        dev_reg |= (a1 % BOOT_PARAM_HD_HEAD_NR) & 0x0f; /* 24-27 addr bit*/
        a1 = (a1 / BOOT_PARAM_HD_HEAD_NR) & 0x0000ffff;
        nsect_low = a0 | (a1 << 8);
        break;
    case 1: /* LBA28 */
        dev_reg |= DEV_LBA28;
        dev_reg |= (nsect_low >> 24) & 0x0f; /* 24-27 addr bit*/
        break;
    case 2: /* LBA48 */
        dev_reg |= DEV_LBA48;
        cmd |= CMD_RW48;
        break;
    default:
        panic("HD: unknown addring mode.");
    }

    outb(DEV_CTRL, 0);
    outb(FEATURES, 0/*features*/);
    if (hd[dev_nr].addr_mode == 2) {
        outb(SECT_CNT, (sect_cnt >> 8) & 0xff);
        outb(ADDR_0, nsect_low >> 24);
        outb(ADDR_1, nsect_high & 0xff);
        outb(ADDR_2, (nsect_high >> 8) & 0xff);
    }
    outb(SECT_CNT, sect_cnt & 0xff);
    outb(ADDR_0,  nsect_low & 0xff);
    outb(ADDR_1, (nsect_low >> 8) & 0xff);
    outb(ADDR_2, (nsect_low >> 16) & 0xff);
    outb(DEVICE, dev_reg);
    outb(COMMAND, cmd);
}

/* 
    read/write disk
    this function will busy wait until succeed
*/
#define DEFAULT_DISK 0
static int hd_rdwt (int rw, int start_sect, int sect_cnt, void * buffer)
{
    assert(start_sect >= 0 && sect_cnt > 0);

    if (!wait_status(STAT_BUSY, 0, NORM_WAIT_TIME)) {
        printf("HD: read/write failed, drive busy.\n");
        return -1;
    }
    // printf("HD: %s from %d to %d, %d sectors\n", (rw == READ) ? "read" : "write",
        // start_sect, start_sect + sect_cnt - 1, sect_cnt);

    char *p = buffer;

    if (rw == READ) {
        hd_cmd_out(DEFAULT_DISK, CMD_READ, (u32)sect_cnt, start_sect, 0);

        while (sect_cnt--) {
            port_read(DATA, p, 512);
            p += 512;
        }
    }
    else if (rw == WRITE) {
        // NOTE: Do not use REP OUTSW to transfer data.
        // There must be a tiny delay between each OUTSW output u16_t.
        // A jmp $+2 size of delay.

        hd_cmd_out(DEFAULT_DISK, CMD_WRITE, (u32)sect_cnt, start_sect, 0);

        while (sect_cnt--) {
            if (!wait_status(STAT_DRQ, STAT_DRQ, NORM_WAIT_TIME))
                panic("HD: writing error.");

            port_write(DATA, p, 512);
            p += 512;
        }
    }
    else
        panic("HD: unknown cmd");

    return 0;
}

void on_hd_interrupt (int int_nr)
{
}

void hd_init (void)
{
    set_irq_handler(IRQ14_ATWIN, on_hd_interrupt);
    enable_irq(IRQ2_8259_SLAVE);
    enable_irq(IRQ14_ATWIN);

    hd_identity(0);
    hd_identity(1);
}

void hd_handle_request (struct bio_request * head)
{
    klock();
    while (head) {
        hd_rdwt(head->cmd.rw, head->cmd.sector,
            head->cmd.count, (void *)(head->cmd.buffer));
        // wake_up
        head = head->next;
    }
    kunlock();
}
