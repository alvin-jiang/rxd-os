#include "blk.h"
#include "assert.h"
#include "stddef.h"

static int NR_BUFFERS;
static struct hlist_head cached_list;
static struct list_head free_list;

struct buffer_head * bread(dev_t dev, sector_t blk_nr)
{
    struct buffer_head *cur = NULL;
    struct hlist_node *hpos, *hn;
    struct buffer_head *n;

    hlist_for_each_entry(cur, hpos, &cached_list, b_hash) {
        if (cur->b_uptodate && cur->b_dev == dev &&
            cur->b_blocknr == blk_nr) {
            printf("bread: %d:%d [hit]\n", dev, blk_nr);
            ++(cur->b_count);
            return cur;
        }
    }

    /* get first entry in free_list */
    cur = NULL;
    if (list_empty(&free_list)) {
        hlist_for_each_entry_safe(cur, hpos, hn, &cached_list, b_hash) {
            if (cur->b_count == 0) {
                hlist_del(&cur->b_hash);
                list_add(&cur->b_free, &free_list);
            }
        }
        if (list_empty(&free_list)) {
            panic("Buffer: cache pool full");
        }
    }
    else {
        list_for_each_entry_safe(cur, n, &free_list, b_free) {
            break;
        }
    }
    assert(cur != NULL && cur->b_count == 0);
    list_del(&cur->b_free);
    hlist_add_head(&cur->b_hash, &cached_list);

    /* read data */
    cur->b_count = 1;
    cur->b_dev = dev;
    cur->b_blocknr = blk_nr;
    ll_rw_block(cur->b_dev, BIO_READ, cur->b_blocknr * SECTORS_PER_BLK, SECTORS_PER_BLK, cur->b_data);
    cur->b_uptodate = 1;
    printf("bread: %d:%d\n", dev, blk_nr);

    return cur;
}
void brelease(struct buffer_head *bh)
{
    --(bh->b_count);
    assert(bh->b_count >= 0);
    if (bh->b_count == 0) {
        if (bh->b_dirty == 1) {
            printf("brelse: %d:%d [sync]\n", bh->b_dev, bh->b_blocknr);
            ll_rw_block(bh->b_dev, BIO_WRITE, bh->b_blocknr * SECTORS_PER_BLK, SECTORS_PER_BLK, bh->b_data);
            bh->b_dirty = 0;
        }
        else
            printf("brelse: %d:%d\n", bh->b_dev, bh->b_blocknr);
    }
}
void bwrite(struct buffer_head *bh)
{
    bh->b_dirty = 1;
}

void buffer_stat(void)
{
    int free = 0;
    struct list_head *pos;
    list_for_each(pos, &free_list) {
        ++free;
    }
    printf("[buffer stat] %d free / %d total\n", free, NR_BUFFERS);
}

void buffer_init(long start, long end)
{
    printf("init buffer module...\n");
    // assert( (end - start) >= (sizeof(struct buffer_head) + BLOCK_SIZE) );

    struct buffer_head *buffer = (struct buffer_head *)start;
    char *block = (char *)end - BLOCK_SIZE;

    INIT_LIST_HEAD(&free_list);
    INIT_HLIST_HEAD(&cached_list);
    while(((long)block - (long)buffer) >= (long)sizeof(struct buffer_head)) {
        buffer->b_uptodate = 0;
        buffer->b_data = block;
        INIT_HLIST_NODE(&buffer->b_hash);
        list_add(&buffer->b_free, &free_list);
        ++buffer;
        block -= BLOCK_SIZE;
    }

    NR_BUFFERS = ((long)buffer - (long)start) / sizeof(struct buffer_head);

    buffer_stat();
    // printf("[buffer stat]: %d buffers, buffer_head size = %d B, block size = %d B, hole = %d B\n",
    //     NR_BUFFERS, sizeof(struct buffer_head), BLOCK_SIZE,
    //     (long)block - (long)buffer + BLOCK_SIZE);
}