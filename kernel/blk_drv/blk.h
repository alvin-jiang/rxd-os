#ifndef __BLK_H__
#define __BLK_H__

#include "type.h"
#include "list.h"

struct buffer_head {
    dev_t b_dev;
    sector_t b_blocknr;

    int b_uptodate;
    char *b_data;

    int b_dirty;
    int b_count;
    int b_lock;
    struct task_struct *b_wait;

    struct buffer_head *b_prev_hash;
    struct buffer_head *b_next_hash;

    struct hlist_node b_hash;
    struct list_head b_free;
};
// b_uptodate, b_dirty, b_lock -> b_state

#define SECTOR_SIZE 512
#define SECTOR_BITS (SECTOR_SIZE << 3)

#define BLOCK_SIZE 1024
#define SECTORS_PER_BLK 2

extern void buffer_init(long start, long end);
extern void buffer_stat(void);
extern struct buffer_head * bread(dev_t dev, sector_t block);
extern void brelease(struct buffer_head *bh);
extern void bwrite(struct buffer_head *bh);

extern void ll_rw_block (dev_t dev, int rw, u32 block, u32 count, char * buffer);

struct bio_cmd {
    int rw;
    u32 sector;
    u32 count;
    char * buffer;
};

/*
    block I/O request
    
*/
struct bio_request {
    struct bio_cmd cmd;
    int dev;
    struct task_struct * waiting;
    struct bio_request * next;
};

struct blk_dev {
    void (*bd_init)(void);
    void (*bd_req_handle)(struct bio_request *);
    struct bio_request * bd_request;
};

#define NR_REQUEST  32
#define NR_BLK_DEV  4
#define BIO_READ    0
#define BIO_WRITE   1

#define BLK_DEV_NULL    0
#define BLK_DEV_HD      3

extern struct blk_dev blk_dev[NR_BLK_DEV];

extern void blk_dev_init (void);

/* hard disk specific */
extern void hd_init (void);
extern void hd_handle_request (struct bio_request * head);
extern void on_hd_interrupt (int int_nr);

// struct blk_dev_struct {
//     void (*request_fn)(void);
//     struct request * current_request;
// };

// #define NR_REQUEST  32
// #define NR_BLK_DEV  7

// extern struct blk_dev_struct blk_dev[NR_BLK_DEV];
// extern struct request request[NR_REQUEST];
// extern struct task_struct * wait_for_request;

/*
#define IN_ORDER(s1,s2) \
((s1)->cmd<(s2)->cmd || (s1)->cmd==(s2)->cmd && \
((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \
(s1)->sector < (s2)->sector)))
*/

#endif
