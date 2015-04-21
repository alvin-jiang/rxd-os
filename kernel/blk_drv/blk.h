

typedef int (*blkdrv_open) (int);
typedef int (*blkdrv_close) (int);
typedef int (*blkdrv_read) (int, char *, int);
typedef int (*blkdrv_write) (int, const char *, int);
typedef int (*blkdrv_ioctl) ();

typedef struct blk_drv
{
    blkdrv_open open;
    blkdrv_close close;
    blkdrv_read read;
    blkdrv_write write;
    blkdrv_ioctl ioctl;
} blk_drv;

static blk_drv block_drivers[] = {
    {}, // hard disk
    {}, // floppy
    {}, // ram disk
};
