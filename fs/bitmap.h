#define _set_bit(addr, offset) do { \
    *((addr) + (offset) / 8) |= (1 << ((offset) % 8)); \
} while (0)

void _set_n_bit(char *addr, int count) {
    int bytes = count / 8;
    int bits = count % 8;
    while ((--bytes) >= 0) {
        *(addr++) = 0xff;
    }
    if (bits) {
        *addr = 1;
        while (--bits) {
            *addr = ((*addr) << 1) | 1;
        }
    }
}

#define _unset_bit(addr, offset) do { \
    *((addr) + (offset) / 8) &= ~(1 << ((offset) % 8)); \
} while (0)
#define _test_bit(addr, offset) \
    ( ( *((addr) + (offset) / 8) & (1 << ((offset) % 8)) ) ? 1 : 0 )

#define _blk_nr(origin, offset) \
    ((origin) + (offset) / SECTOR_BITS)
#define _bit_nr(offset) \
    ((offset) % SECTOR_BITS)

static long _find_first_zero(void * addr, int limit)
{
    long zero_offset = 0;
    unsigned char *first = (unsigned char *)addr;
    unsigned char *end = first + limit;
    unsigned char tmp;
    while (first != end) {
        if (*first == 0xff)
            ++first;
        else {
            zero_offset = ((long)first - (long)addr) << 3;
            tmp = *first;
            while (tmp & 1) {
                tmp >>= 1;
                ++zero_offset;
            }
            return zero_offset;
        }
    }
    return -1;
}

static long _bitmap_alloc(dev_t dev, sector_t start, sector_t end)
{
    struct buffer_head *bh = NULL;
    long bit = 0;
    long tmp;
    while (start != end) {
        bh = bread(dev, start);
        tmp = _find_first_zero(bh->b_data, SECTOR_SIZE);
        if (tmp >= 0) {
            bit += tmp;
            _set_bit(bh->b_data, _bit_nr(bit));
            bwrite(bh);
            brelease(bh);
            break;
        }
        else {
            bit += SECTOR_BITS;
            ++start;
            brelease(bh);
        }
    }

    if (start == end)
        panic("_bitmap_alloc: map is full");

    return (start == end) ? 0 : bit;
}

static void _bitmap_free(dev_t dev, sector_t start, sector_t end, long bit)
{
    if (bit < 0 || bit >= ((end - start) * SECTOR_BITS))
        panic("_bitmap_free: invalid bit");

    struct buffer_head *bh = NULL;
    bh = bread(dev, _blk_nr(start, bit));
    _unset_bit(bh->b_data, _bit_nr(bit));
    bwrite(bh);
    brelease(bh);
}

static int _bitmap_test(dev_t dev, sector_t start, sector_t end, long bit)
{
    // printf("_bitmap_test: start = %d end = %d bit = %d\n", start, end, bit);
    if (bit < 0 || bit >= ((end - start) * SECTOR_BITS)) {
        printf("_bitmap_test: start = %d end = %d bit = %d\n", start, end, bit);
        panic("_bitmap_test: invalid bit");
    }

    int test;
    struct buffer_head *bh = NULL;

    bh = bread(dev, _blk_nr(start, bit));
    test = _test_bit(bh->b_data, _bit_nr(bit));
    brelease(bh);

    return test;
}

#define _alloc_block_nr(sb) \
    _bitmap_alloc((sb)->s_dev, (sb)->s_zmap_start, \
        (sb)->s_zmap_start + (sb)->s_zmap_count)
#define _free_block_nr(sb, bit) \
    _bitmap_free((sb)->s_dev, (sb)->s_zmap_start, \
        (sb)->s_zmap_start + (sb)->s_zmap_count, bit)
#define _test_block_nr(sb, bit) \
    _bitmap_test((sb)->s_dev, (sb)->s_zmap_start, \
        (sb)->s_zmap_start + (sb)->s_zmap_count, bit)

#define alloc_inode_nr(sb) \
    _bitmap_alloc((sb)->s_dev, (sb)->s_imap_start, \
        (sb)->s_imap_start + (sb)->s_imap_count)
#define free_inode_nr(sb, bit) \
    _bitmap_free((sb)->s_dev, (sb)->s_imap_start, \
        (sb)->s_imap_start + (sb)->s_imap_count, bit)
#define test_inode_nr(sb, bit) \
    _bitmap_test((sb)->s_dev, (sb)->s_imap_start, \
        (sb)->s_imap_start + (sb)->s_imap_count, bit)
