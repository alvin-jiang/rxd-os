/*
    We only support primary IDE channel now.
*/

// #include "blk.h"

#include "head.h"
#include "stdio.h"
#include "assert.h"

#include "hd.h"

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
#define IS_DRIVE_READY(stat) \
    (!((stat) & STAT_BUSY))
//  (((stat) & (STAT_BUSY | STAT_DRQ | STAT_READY)) == STAT_READY)

#define ALT_STATUS  DEV_CTRL

// helper marcos
#define IS_ATA() (!in_byte(0x1f4) && !in_byte(0x1f5))

typedef struct hd_dev
{
    int     addr_mode; /* 0:C/H/S 1:LBA28 2:LBA48 */
    uint32  sect_cnt;
    uint32  sect_cnt_ex;
} hd_dev;

static hd_dev hd[2] = {{0, 0, 0}, {0, 0, 0}};

int hd_signal;

static int wait_status (int stat, int value)
{
    int retries = 10000;

    while (--retries && (in_byte(STATUS) & stat) != value);
    return retries;
}

/* retrieve hard disk information */
static void hd_identity (uint8 dev_nr)
{
    assert( dev_nr <= 1 );

    sti();
    hd_signal = 0;
    out_byte(DEV_CTRL, 0);
    out_byte(SECT_CNT, 0);
    out_byte(ADDR_0,  0);
    out_byte(ADDR_1, 0);
    out_byte(ADDR_2, 0);
    out_byte(DEVICE, (dev_nr << 4) | DEV_CHS);
    out_byte(COMMAND, CMD_IDENTIFY);
    if (!in_byte(STATUS)) {
        printf("HD: disk %d not found\n", dev_nr);
        cli();
        return;
    }

    while (!hd_signal);
    cli();

    /* read hard disk info */
    uint16 buf[256];
    port_read(DATA, buf, sizeof(buf));
    hd[dev_nr].addr_mode = (buf[49] & 0x0200) ? ((buf[83] & 0x400) ? 2 : 1) : 0;
    if (hd[dev_nr].addr_mode == 1)
        hd[dev_nr].sect_cnt = (uint32)(buf[61] << 16) | buf[60];
    else if (hd[dev_nr].addr_mode == 2) {
        hd[dev_nr].sect_cnt = (uint32)(buf[101] << 16) | buf[100];
        hd[dev_nr].sect_cnt_ex = (uint32)(buf[103] << 16) | buf[102];
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
static void hd_cmd_out (uint8 dev_nr, uint8 cmd,/* uint8 features, */
    uint32 sect_cnt, uint32 nsect_low, uint32 nsect_high)
{
    assert( dev_nr <= 1 );

    uint32 a0, a1;
    uint8 dev_reg;

    // miss limit check
    if (!sect_cnt | sect_cnt > (hd[dev_nr].addr_mode == 2 ? 65536 : 256))
        panic("HD: invalid sector count.");
    if (nsect_high != 0 | (nsect_low >= 0x10000000) )
        panic("HD: sorry, the largest capacity we support now is 128GB.");
    if (!wait_status(STAT_BUSY, 0))
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

    out_byte(DEV_CTRL, 0);
    out_byte(FEATURES, 0/*features*/);
    if (hd[dev_nr].addr_mode == 2) {
        out_byte(SECT_CNT, (sect_cnt >> 8) & 0xff);
        out_byte(ADDR_0, nsect_low >> 24);
        out_byte(ADDR_1, nsect_high & 0xff);
        out_byte(ADDR_2, (nsect_high >> 8) & 0xff);
    }
    out_byte(SECT_CNT, sect_cnt & 0xff);
    out_byte(ADDR_0,  nsect_low & 0xff);
    out_byte(ADDR_1, (nsect_low >> 8) & 0xff);
    out_byte(ADDR_2, (nsect_low >> 16) & 0xff);
    out_byte(DEVICE, dev_reg);
    out_byte(COMMAND, cmd);
}

void hd_init (void)
{
    set_int_callback(14, on_hd_interrupt);
    enable_int(2);
    enable_int(14);

    hd_identity(0);
    hd_identity(1);
}

void on_hd_interrupt (int int_nr)
{
    hd_signal = 1;
}


int hd_rdwt (int rw, int start_sect, int sect_cnt, void * buffer)
{
    if (!wait_status(STAT_BUSY, 0)) {
        printf("HD: read/write failed, drive busy.\n");
        return -1;
    }
    // NOTE: Do not use REP OUTSW to transfer data.
    // There must be a tiny delay between each OUTSW output uint16_t.
    // A jmp $+2 size of delay.
    int i;
    char *p = buffer;

    if (!rw) { // read
        hd_signal = 0;
        sti();
        hd_cmd_out(0, CMD_READ, (uint32)sect_cnt, 0, 0);
        while (!hd_signal);
        cli();
        while (sect_cnt--) {
            port_read(DATA, p, 512);
            for (i = 0; i < 30; ++i)
                printf("%x ", p[i]);
            printf("\n");
            p += 512;
        }
    }
    else { // write
        sti();
        hd_signal = 0;

        hd_cmd_out(0, CMD_WRITE, (uint32)sect_cnt, 0, 0);

        while (sect_cnt--) {
            if (!wait_status(STAT_DRQ, STAT_DRQ))
                panic("HD: writing error.");

            port_write(DATA, p, 512);
            p += 512;

            while (!hd_signal);
            hd_signal = 0;
        }
        cli();
    }
    return 0;
}

// void print_identify_info(uint16 * hdinfo)
// {
//     int i, k;
//     char s[64];

//     struct iden_info_ascii {
//         int idx;
//         int len;
//         char * desc;
//     } iinfo[] = {{10, 20, "HD SN"}, /* Serial number in ASCII */
//              {27, 40, "HD Model"} /* Model number in ASCII */ };

//     for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++) {
//         char * p = (char*)&hdinfo[iinfo[k].idx];
//         for (i = 0; i < iinfo[k].len/2; i++) {
//             s[i*2+1] = *p++;
//             s[i*2] = *p++;
//         }
//         s[i*2] = 0;
//         printf("%s: %s\n", iinfo[k].desc, s);
//     }

//     int capabilities = hdinfo[49];
//     printf("LBA supported: %s\n",
//            (capabilities & 0x0200) ? "Yes" : "No");

//     int cmd_set_supported = hdinfo[83];
//     printf("LBA48 supported: %s\n",
//            (cmd_set_supported & 0x0400) ? "Yes" : "No");

//     int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
//     printf("HD size: %dMB\n", sectors * 512 / 1000000);
// }

