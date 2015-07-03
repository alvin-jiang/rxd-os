/**
 *
 * @file: fs.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-05-20
 *
 */

#include "fs.h"
#include "proc.h"
#include "blk.h"
#include "string.h"
#include "assert.h"
#include "string.h"
#include "mm.h"
#include "entry.h"

#define SUPER_MAGIC (0x12345678)

#include "bitmap.h"

static struct super_block super;
static struct inode root_inode;
static struct dentry root_dentry;

static struct list_head inode_list = LIST_HEAD_INIT(inode_list);
static struct list_head dentry_list = LIST_HEAD_INIT(dentry_list);

static struct kmem_cache *inode_cache = NULL;
static struct kmem_cache *dname_cache = NULL;
static struct kmem_cache *dentry_cache = NULL;
static struct kmem_cache *file_cache = NULL;

static void dump_inode(struct inode *inode);
static int _reserve_zone(struct inode *inode, sector_t nzone, int truncate);

/* file system resource allocation */
static sector_t _get_blk(struct super_block *sb)
{
    assert(sb == &super);
    sector_t blk_nr = _alloc_block_nr(sb);

    /* clear block content */
    if (blk_nr != 0) {
        struct buffer_head *bh = bread(sb->s_dev, blk_nr);
        memset(bh->b_data, 0, BLOCK_SIZE);
        bwrite(bh);
        brelease(bh);
    }
    return blk_nr;
}
static void _put_blk(struct super_block *sb, sector_t blk_nr)
{
    _free_block_nr(sb, blk_nr);
}

/* inode create/set/get */
struct inode * create_inode(struct super_block *sb, unsigned short mode)
{
    assert(sb == &super);

    long inum;
    struct inode *inode;

    if ((inum = alloc_inode_nr(sb)) == -1) {
        printf("create_inode: failed to allocate inode number\n");
        return NULL;
    }
    if ((inode = (struct inode *)kmem_cache_alloc(inode_cache)) == NULL) {
        printf("create_inode: failed to allocate inode body\n");
        return NULL;
    }

    inode->i_inum = (unsigned long)inum;
    inode->i_size = 0;
    inode->i_mode = mode;
    inode->i_nlink = 0; // TODO: if inode is dir, i_nlink = 2
    memset(inode->i_zone, 0, sizeof(inode->i_zone[0]) * 9);
    inode->i_super = sb;
    INIT_LIST_HEAD(&inode->i_dentry);

    inode->i_refcnt = 1;
    list_add(&inode->i_list, &inode_list);

    /* write inode struct */
    write_inode(inode);
    return inode;
}
struct inode * get_inode(struct super_block *sb, unsigned long inum)
{
    struct inode *inode = NULL;
    list_for_each_entry(inode, &inode_list, i_list) {
        if (inode->i_inum == inum && inode->i_refcnt != 0) {
            printf("get_inode(): %d hit\n", inum);
            ++(inode->i_refcnt);
            return inode;
        }
    }

    if ((inode = (struct inode *)kmem_cache_alloc(inode_cache)) == NULL) {
        printf("get_inode(): failed to allocate inode body\n");
        return NULL;
    }
    read_inode(inode, inum, sb);
    list_add(&inode->i_list, &inode_list);

    printf("get_inode(): %d miss\n", inum);
    ++(inode->i_refcnt);
    return inode;
}
void put_inode(struct inode *inode)
{
    assert(inode && inode->i_refcnt > 0);

    struct super_block *sb = inode->i_super;
    assert(sb == &super);

    if (--(inode->i_refcnt) == 0) {
        // if inode is dirty
        if (inode->i_nlink == 0) {
            printf("put_inode(): delete %d\n", inode->i_inum);
            assert(list_empty(&inode->i_dentry) && inode != &root_inode);
            _reserve_zone(inode, 0, 1);
            free_inode_nr(sb, inode->i_inum);
        }
        else {
            printf("put_inode(): sync %d\n", inode->i_inum);
            write_inode(inode);
        }
        list_del(&inode->i_list);
        kmem_cache_free(inode_cache, inode);
    }
}

/* inode loader/unloader */
int read_inode(struct inode *inode, unsigned long inum, struct super_block *sb)
{
    assert(inum && sb == &super);

    size_t inode_per_blk;
    sector_t blknr;
    size_t blkoff;
    struct inode d_inode;

    /* does inode exist ? */
    if (!test_inode_nr(sb, inum))
        panic("read non-exist inode");
    printf("read_inode(): read %d\n", inum);

    inode_per_blk = BLOCK_SIZE / sizeof(struct inode);
    blknr = (sector_t)(inum / inode_per_blk) + sb->s_itable_start;
    blkoff = (size_t)(inum % inode_per_blk) * sizeof(struct inode);

    struct buffer_head *bh = bread(sb->s_dev, blknr);
    memcpy(&d_inode, bh->b_data + blkoff, sizeof(struct inode));
    brelease(bh);

    inode->i_inum = inum;
    inode->i_super = sb;
    assert(inode->i_inum == d_inode.i_inum);
    inode->i_size = d_inode.i_size;
    inode->i_mode = d_inode.i_mode;
    inode->i_nlink = d_inode.i_nlink;
    memcpy(inode->i_zone, d_inode.i_zone, 9 * sizeof(sector_t));
    return 0;
}
void write_inode(struct inode *inode)
{
    assert(inode && inode->i_inum && inode->i_super == &super);

    struct super_block *sb = inode->i_super;
    unsigned long inum = inode->i_inum;

    size_t inode_per_blk;
    sector_t blknr;
    size_t blkoff;

    /* does inode exist ? */
    if (!test_inode_nr(sb, inum))
        panic("write non-exist inode");
    printf("write_inode(): write %d\n", inum);

    inode_per_blk = BLOCK_SIZE / sizeof(struct inode);
    blknr = (sector_t)(inum / inode_per_blk) + sb->s_itable_start;
    blkoff = (size_t)(inum % inode_per_blk) * sizeof(struct inode);

    struct buffer_head *bh = bread(sb->s_dev, blknr);
    memcpy(bh->b_data + blkoff, inode, sizeof(struct inode));
    bwrite(bh);
    brelease(bh);
}
/* inode zone helpers */
static void _set_zone_blk(struct inode *inode, sector_t zone_nr, sector_t blk_nr)
{
    assert(zone_nr >= 0 && zone_nr < 7 + 256 + 256 * 256);
    if (zone_nr > 6)
        panic("Indirect inode zone is not support now");
    inode->i_zone[zone_nr] = blk_nr;
}
static sector_t _get_zone_blk(struct inode *inode, sector_t zone_nr)
{
    assert(zone_nr >= 0 && zone_nr < 7 + 256 + 256 * 256);
    if (zone_nr > (inode->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE)
        panic("_get_zone_blk(): invalid zone number, too big.");

    if (zone_nr > 6)
        panic("Indirect inode zone is not support now");
    return inode->i_zone[zone_nr];
}
/*
    reserve enough zones for inode
    TODO: it should rollback all things(e.g. allocated blocks) if failed.
    return 0 if succeed
*/
static int _reserve_zone(struct inode *inode, sector_t nzone, int truncate)
{
    /* we need to assume this file contains no hole */
    assert(inode && inode->i_inum >= 1);
    sector_t cur_nzone = (inode->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    sector_t blk;
    if (cur_nzone < nzone) {
        do {
            /* TODO: we should rollback all block allocations if failed. */
            _set_zone_blk(inode, cur_nzone, _get_blk(inode->i_super));
            // printf("\talloc izone[%d]: %d\n", cur_nzone, _get_zone_blk(inode, cur_nzone));
            ++cur_nzone;
        } while (cur_nzone != nzone);
    }
    else if (cur_nzone > nzone && truncate) {
        do {
            --cur_nzone;
            blk = _get_zone_blk(inode, cur_nzone);
            _set_zone_blk(inode, cur_nzone, 0);
            _put_blk(inode->i_super, blk);
            // printf("\tfree izone[%d]\n", cur_nzone);
        } while (cur_nzone != nzone);
    }
    return 0;
}
/* inode zone read/write */
// return read count, or -1 if failed
static int _read(struct inode *inode, int pos, char * buf, int count)
{
    assert(pos >= 0 && count >= 0);
    struct buffer_head *bh = NULL;
    sector_t zone;
    size_t left_bytes;
    size_t bytes;

    // read can't exceed file end
    if ((pos + count) > inode->i_size)
        return -1;

    left_bytes = count;
    zone = pos / BLOCK_SIZE;

    bytes = BLOCK_SIZE - (pos % BLOCK_SIZE);
    bytes = left_bytes < bytes ? left_bytes : bytes;
    bh = bread(inode->i_super->s_dev, _get_zone_blk(inode, zone));
    memcpy(buf, bh->b_data + (pos % BLOCK_SIZE), bytes);
    brelease(bh);

    buf += bytes;
    left_bytes -= bytes;
    ++zone;
    while (left_bytes > 0) {
        bytes = (left_bytes > BLOCK_SIZE) ? left_bytes % BLOCK_SIZE : left_bytes;
        bh = bread(inode->i_super->s_dev, _get_zone_blk(inode, zone));
        memcpy(buf, bh->b_data, bytes);
        brelease(bh);
        buf += bytes;
        left_bytes -= bytes;
        ++zone;
    }

    return (count - left_bytes);
}
// return write count, or -1 if failed
static int _write(struct inode *inode, int pos, const char * buf, int count)
{
    assert(pos >= 0 && count > 0);
    struct buffer_head *bh = NULL;
    sector_t zone;
    size_t left_bytes;
    size_t bytes;

    // make sure we have enough zones
    if (_reserve_zone(inode, (pos + count + BLOCK_SIZE - 1) / BLOCK_SIZE, 0))
        return -1;

    left_bytes = count;
    zone = pos / BLOCK_SIZE;

    bytes = BLOCK_SIZE - (pos % BLOCK_SIZE);
    bytes = left_bytes < bytes ? left_bytes : bytes;
    bh = bread(inode->i_super->s_dev, _get_zone_blk(inode, zone));
    memcpy(bh->b_data + (pos % BLOCK_SIZE), buf, bytes);
    bwrite(bh);
    brelease(bh);

    buf += bytes;
    left_bytes -= bytes;
    ++zone;
    while (left_bytes > 0) {
        bytes = (left_bytes > BLOCK_SIZE) ? left_bytes % BLOCK_SIZE : left_bytes;
        bh = bread(inode->i_super->s_dev, _get_zone_blk(inode, zone));
        memcpy(bh->b_data, buf, bytes);
        bwrite(bh);
        brelease(bh);
        buf += bytes;
        left_bytes -= bytes;
        ++zone;
    }

    /* update inode size if necessary */
    if (inode->i_size < (pos + count - left_bytes)) {
        inode->i_size = pos + count - left_bytes;
    }
    write_inode(inode);
    return (count - left_bytes);
}
static int _find_dir_entry(struct inode *dir, struct dir_entry *target)
{
    struct dir_entry de;
    int pos = 0;

    while (_read(dir, pos, (char *)&de, DIR_REC_LEN) == DIR_REC_LEN) {
        if (!strncmp(target->name, de.name, target->name_len)) {
            target->inode = de.inode;
            target->rec_len = de.rec_len;
            target->file_type = de.file_type;
            break;
        }
        pos += DIR_REC_LEN;
    }
    return pos < dir->i_size ? pos : -1;
}
static int _add_dir_entry(struct inode *dir, struct dir_entry *target)
{
    struct dir_entry de;
    int pos = 0;

    // backup target
    memcpy(&de, target, DIR_REC_LEN);
    if (_find_dir_entry(dir, &de) >= 0) {
        printf("_add_dir_entry(): dir entry '%s' already exists.\n", target->name);
        return -1;
    }

    while (_read(dir, pos, (char *)&de, DIR_REC_LEN) == DIR_REC_LEN) {
        if (de.inode == 0) {
            break;
        }
        pos += DIR_REC_LEN;
    }
    _write(dir, pos, (const char *)target, DIR_REC_LEN);
    ++(dir->i_nlink);
    write_inode(dir);
    return 0;
}
/*static int _remove_dir_entry(struct inode *dir, struct dir_entry *target)
{
    struct dir_entry de;
    int pos = 0;

    // backup target
    memcpy(&de, target, DIR_REC_LEN);
    if ((pos = _find_dir_entry(dir, &de)) < 0) {
        printf("_remove_dir_entry(): dir entry '%s' not found.\n", target->name);
        return -1;
    }
    memset(&de, 0, DIR_REC_LEN);
    _write(dir, pos, (const char *)&de, DIR_REC_LEN);
    --(dir->i_nlink);
    write_inode(dir);
    return 0;
}*/

/* super block read/write */
static struct super_block * read_super(dev_t dev)
{
    struct buffer_head *bh;
    struct super_block *sb = &super;

    bh = bread(dev, 1);
    memcpy(sb, bh->b_data, sizeof(struct super_block));
    brelease(bh);

    sb->s_dev = dev;
    return sb;
}

static void write_super(struct super_block *sb)
{
    struct buffer_head *bh = bread(sb->s_dev, 1);
    memcpy(bh->b_data, sb, sizeof(struct super_block));
    bwrite(bh);
    brelease(bh);
}

/* dentry helpers */
static int _qstrcmp(struct qstr * name1, struct qstr * name2)
{
    if (name1->len == name2->len)
        return strncmp(name1->name, name2->name, name1->len);
    else
        return (name1->len > name2->len) ? 1 : -1;
}

/* path: [a-z]+{/[a-z]+}* */
#define path_for_each(dname, path) \
    for ((dname)->name = (path), (dname)->len = strcspn((path), "/"), \
            (dname)->len = (dname)->len ? (dname)->len : 1; \
        *(dname)->name; \
        (dname)->name += (dname)->len, \
            (dname)->name += (*(dname)->name == '/') ? 1 : 0, \
            (dname)->len = strcspn((dname)->name, "/"))

#define path_for_each_dir(dname, path) \
    for ((dname)->name = (path), (dname)->len = strcspn((path), "/"), \
            (dname)->len = (dname)->len ? (dname)->len : 1; \
        *((dname)->name + (dname)->len); \
        (dname)->name += (dname)->len, \
            (dname)->name += (*(dname)->name == '/') ? 1 : 0, \
            (dname)->len = strcspn((dname)->name, "/"))


static inline void __dlink_inode(struct dentry *dentry, struct inode *inode)
{
    dentry->d_inode = inode;
    ++(dentry->d_refcnt);
    list_add(&dentry->d_alias, &inode->i_dentry);
    ++(inode->i_refcnt);
}
static inline void __dunlink_inode(struct dentry *dentry)
{
    list_del(&dentry->d_alias);
    --(dentry->d_inode->i_refcnt);
    dentry->d_inode = NULL;
    --(dentry->d_refcnt);
}
static inline void __dlink_parent(struct dentry *dentry, struct dentry *parent)
{
    dentry->d_parent = parent;
    ++(dentry->d_refcnt);
    ++(parent->d_refcnt);
}
static inline void __dunlink_parent(struct dentry *dentry)
{
    --(dentry->d_parent->d_refcnt);
    dentry->d_parent = NULL;
    --(dentry->d_refcnt);
}

// create tree node
struct dentry * create_dentry(struct dentry *parent, struct qstr *dname, struct inode *inode)
{
    /* check params */
    // parent is dir
    // dname is valid
    struct inode *dir = parent->d_inode;
    struct dentry *dentry;

    /* check if dentry already exists */
    if (get_dentry(parent, dname) != NULL) {
        printf("create_dentry(): dentry '%s' already exists.\n", dname->name);
        return NULL;
    }

    /* create dentry */
    struct dir_entry de = {
        inode->i_inum,
        DIR_REC_LEN,
        dname->len,
        FT_REG_FILE,
    };
    assert(dname->len < MAX_DENTRY_LENGTH);
    strncpy(de.name, dname->name, MAX_DENTRY_LENGTH);
    _add_dir_entry(dir, &de);

    dentry = (struct dentry *)kmem_cache_alloc(dentry_cache); assert(dentry);
    dentry->d_name.len = dname->len;
    dentry->d_name.name = (const char *)kmem_cache_alloc(dname_cache); assert(dentry->d_name.name);
    memset((void *)dentry->d_name.name, 0, MAX_DENTRY_LENGTH);
    strncpy((char *)dentry->d_name.name, dname->name, dname->len);

    dentry->d_refcnt = 0;
    list_add(&dentry->d_list, &dentry_list);
    __dlink_parent(dentry, parent);
    __dlink_inode(dentry, inode);

    ++(inode->i_nlink);
    write_inode(inode);

    printf("create_dentry(): create '%s'\n", dname->name);

    return dentry;
}
struct dentry * get_dentry(struct dentry *parent, struct qstr *dname)
{
    // assert dir is valid and is directory
    // assert(dir && !list_empty(&dir->i_dentry) &&
    //     dir->i_dentry.next->next == &dir->i_dentry);
    // assert dentry->d_name is valid
    struct inode *dir = parent->d_inode;
    struct super_block *sb = dir->i_super;
    struct dentry *dentry = NULL;
    struct inode *inode = NULL;

    /* try searching cache */
    list_for_each_entry(dentry, &dentry_list, d_list) {
        if (!_qstrcmp(&dentry->d_name, dname)
            && dentry->d_refcnt != 0
            && dentry->d_parent == parent) {
            ++(dentry->d_refcnt);
            printf("get_dentry(): hit '%s' %d\n", dname->name, dname->len);
            return dentry;
        }
    }
    dentry = NULL;

    /* search disk */
    assert(dname->len < MAX_DENTRY_LENGTH);
    struct dir_entry de;
    int pos;

    de.name_len = dname->len;
    strncpy(de.name, dname->name, MAX_DENTRY_LENGTH);
    if ((pos = _find_dir_entry(dir, &de)) < 0) {
        printf("get_dentry(): not found '%s' %d\n", dname->name, dname->len);
        return NULL;
    }

    if ((inode = get_inode(sb, de.inode)) == NULL) {
        panic("get_dentry(): dentry has no inode");
    }
    if ((dentry = (struct dentry *)kmem_cache_alloc(dentry_cache)) == NULL) {
        printf("get_dentry(): failed to allocate dentry\n");
        put_inode(inode);
        return NULL;
    }

    dentry->d_name.len = dname->len;
    dentry->d_name.name = (const char *)kmem_cache_alloc(dname_cache);
    memset((void *)dentry->d_name.name, 0, MAX_DENTRY_LENGTH);
    strncpy((char *)dentry->d_name.name, dname->name, dname->len);

    dentry->d_refcnt = 0;
    list_add(&dentry->d_list, &dentry_list);
    __dlink_parent(dentry, parent);
    __dlink_inode(dentry, inode);
    printf("get_dentry(): miss '%s' %d\n", dname->name, dname->len);

    return dentry;
}
void put_dentry(struct dentry *dentry)
{
    assert( dentry && dentry->d_refcnt > 0 );
    // save inode pointer
    struct inode *inode = dentry->d_inode;
    __dunlink_parent(dentry);
    __dunlink_inode(dentry);
    if (dentry->d_refcnt == 0) {
        put_inode(inode);
        list_del(&dentry->d_list);
        kmem_cache_free(dname_cache, (char *)dentry->d_name.name);
        kmem_cache_free(dentry_cache, dentry);
    }
}

static void _close(struct dentry *dentry)
{
    // assert(dentry is a file)
    struct dentry *parent;

    parent = dentry->d_parent;
    put_dentry(dentry);
    if (dentry->d_refcnt == 0) {
        dentry = parent;
        while (dentry != &root_dentry) {
            parent = dentry->d_parent;
            put_dentry(dentry);
            dentry = parent;
        }
    }
}

static inline struct dentry * _open(const char *path)
{
    // assert path is valid
    struct dentry *dentry = NULL, *child = NULL;
    assert(*path == '/');
    if (*path == '/') {
        dentry = &root_dentry;
        ++root_dentry.d_refcnt;
        ++path;
    }
    else {
        // dentry = current_task->pwd;
        panic("relative path not support yet.");
    }

    struct qstr dname;
    path_for_each(&dname, path) {
        if((child = get_dentry(dentry, &dname)) == NULL) {
            printf("_open(): path '%s' not found\n", path);
            _close(dentry);
            dentry = NULL;
            break;
        }
        else
            dentry = child;
    }
    return dentry;
}
static inline struct dentry * _opendir(const char *path)
{
    // assert path is valid
    struct dentry *dentry = NULL, *child = NULL;
    assert(*path == '/');
    if (*path == '/') {
        dentry = &root_dentry;
        ++root_dentry.d_refcnt;
        ++path;
    }
    else {
        // dentry = current_task->pwd;
        panic("relative path not support yet.");
    }

    struct qstr dname;
    path_for_each_dir(&dname, path) {
        if((child = get_dentry(dentry, &dname)) == NULL) {
            printf("_opendir(): path '%s' not found\n", path);
            _close(dentry);
            dentry = NULL;
            break;
        }
        else
            dentry = child;
    }
    return dentry;
}

// BUG: memory leak!!!!
#define KERNEL_STRING(name, usrstr) \
    char *name = (char *)alloc_page(); \
    strncpy_from_user(name, usrstr, PAGE_SIZE)

int sys_open(const char *_pathname, int flags, mode_t mode)
{
    struct file *filp;
    struct dentry *dentry, *parent;
    KERNEL_STRING(pathname, _pathname);

    /* copy pathname to kernel space */
    /* check if pathname is valid */
    // {pathname} = [/]{de}[/{de}]*
    // {de} = [-_'a-zA-Z0-9]{1, MAX_DENTRY_LENGTH}

    /* get a free slot in task file pointer table */
    struct file **filp_cur = &(current_task->filp[0]);
    struct file **filp_end = filp_cur + NR_MAX_OPEN;
    while (filp_cur != filp_end) {
        if (*filp_cur == NULL)
            break;
        ++filp_cur;
    }
    if (filp_cur == filp_end) {
        return -1;
    }
    /* get a free slot in file table */
    if ((*filp_cur = kmem_cache_alloc(file_cache)) == NULL) {
        printf("sys_open(): alloc file failed\n");
        return -1;
    }
    filp = *filp_cur;
    filp->f_flags = flags;
    filp->f_mode = 0;
    filp->f_pos = 0;

    if ((parent = _opendir(pathname)) == NULL) {
        printf("sys_open(): open '%s' failed, no such file\n", pathname);
        kmem_cache_free(file_cache, filp);
        return -1;
    }
    assert(parent == &root_dentry);
    assert(parent->d_inode == &root_inode);
    assert(parent->d_inode->i_super == &super);

    /* open path */
    struct inode *inode = NULL;
    struct qstr filename;
    filename.name = strrchr(pathname, '/');
    if (filename.name == NULL)
        filename.name = pathname;
    else
        filename.name += 1;
    filename.len = strlen(filename.name);
    printf("sys_open(): check file name '%s'\n", filename.name);

    if ((dentry = get_dentry(parent, &filename)) == NULL) {
        if (flags & O_CREAT) {
            printf("sys_open(): create inode of '%s'\n", filename.name);
            inode = create_inode(parent->d_inode->i_super, 123);
            printf("sys_open(): create dentry of '%s'\n", filename.name);
            dentry = create_dentry(parent, &filename, inode);
            printf("sys_open(): create file '%s'\n", dentry->d_name.name);
        }
        else {
            printf("sys_open(): open '%s' failed, no such file\n", pathname);
            kmem_cache_free(file_cache, filp);
            return -1;
        }
    }
    else {
        printf("sys_open(): find file '%s'\n", dentry->d_name.name);
    }
    filp->f_dentry = dentry;
    filp->f_refcnt = 1;
    return filp_cur - &(current_task->filp[0]);
}

#define _filp(fd) (current_task->filp[fd])

int sys_close(int fd)
{
    struct file *filp = _filp(fd);

    _close(filp->f_dentry);
    kmem_cache_free(file_cache, filp);
    return 0;
}

ssize_t sys_read(int fd, void *usrbuf, size_t count)
{
    struct file *filp = _filp(fd);
    struct inode *inode = filp->f_dentry->d_inode;
    char *buffer = (char *)alloc_page();
    size_t left;
    int bytes;

    // printf("sys_read() dump:\n");
    // fs_dump_cache();
    // dump_inode(inode);

    left = count;
    while (left > 0) {
        bytes = (left > PAGE_SIZE) ? PAGE_SIZE : (int)left;
        bytes = _read(inode, filp->f_pos, buffer, bytes);
        if (bytes > 0) {
            memcpy_to_user(usrbuf, buffer, bytes);
            printf("sys_read(): read at %d, %d bytes, '%s'\n", filp->f_pos, bytes, buffer);
            left -= bytes;
            usrbuf += bytes;
            filp->f_pos += bytes;
        }
        else {
            printf("sys_read(): exceeds file end, truncate %d -> %d\n", count,
                count - left -(inode->i_size - filp->f_pos));
            count -= left -(inode->i_size - filp->f_pos);
            left = inode->i_size - filp->f_pos;
        }
    }

    free_page((unsigned long)buffer);
    return count - left;
}
ssize_t sys_write(int fd, const void *usrbuf, size_t count)
{
    struct file *filp = _filp(fd);
    struct inode *inode = filp->f_dentry->d_inode;
    char *buffer = (char *)alloc_page();
    size_t left;
    int bytes;

    left = count;
    while (left > 0) {
        bytes = (left > PAGE_SIZE) ? PAGE_SIZE : (int)left;
        memcpy_from_user(buffer, usrbuf, bytes);
        printf("sys_write(): write at %d, %d bytes, '%s'\n", filp->f_pos, bytes, buffer);
        bytes = _write(inode, filp->f_pos, buffer, bytes);
        if (bytes > 0) {
            left -= bytes;
            usrbuf += bytes;
            filp->f_pos += bytes;
        }
        else
            break;
    }

    free_page((unsigned long)buffer);
    return count - left;
}

off_t sys_lseek(int fd, off_t offset, int whence)
{
    struct file *filp = _filp(fd);
    off_t ret = 0;
    switch(whence) {
        case SEEK_SET:
            ret = filp->f_pos = offset;
            break;
        case SEEK_CUR:
            ret = (filp->f_pos += offset);
            break;
        case SEEK_END:
            ret = filp->f_pos = filp->f_dentry->d_inode->i_size + offset;
            break;
        default:
            ret = -1;
            break;
    }
    return ret;
}
int sys_truncate(const char *_pathname, off_t length)
{
    KERNEL_STRING(pathname, _pathname);

    struct dentry *dentry = _open(pathname);
    struct inode *inode;
    if (dentry != NULL) {
        inode = dentry->d_inode;
        if (length < inode->i_size) {
            _reserve_zone(inode, (length + BLOCK_SIZE - 1) / BLOCK_SIZE, 1);
            inode->i_size = length;
        }
        return 0;
    }
    else
        return -1;
}
int sys_link(const char *_oldpath, const char *_newpath)
{
    KERNEL_STRING(oldpath, _oldpath);
    KERNEL_STRING(newpath, _newpath);

    struct dentry *old_dentry;
    struct dentry *new_dir, *new_dentry;

    // oldpath must exists, and not a directory
    if ((old_dentry = _open(oldpath)) == NULL) {
        printf("sys_link(): target '%s' not exists.\n", oldpath);
        return -1;
    }
    // newpath dir must exists, file must not exists
    if ((new_dir = _opendir(newpath)) == NULL) {
        printf("sys_link():  link path '%*s' not exists.\n",
            (ptrdiff_t)strrchr(newpath, '/') - (ptrdiff_t)newpath, newpath);
        return -1;
    }
    struct qstr name;
    name.name = strrchr(newpath, '/') + 1;
    name.len = strlen(name.name);
    if ((new_dentry = get_dentry(new_dir, &name)) != NULL) {
        printf("sys_link():  link file '%s' already exists.\n", newpath);
        return -1;
    }

    // new_entry = create_dentry(new_dir, &dname, old_entry->d_inode);
    return (new_dentry == NULL) ? -1 : 0;
}

/*int sys_unlink(const char *_pathname)
{
    KERNEL_STRING(pathname, _pathname);
}
int sys_mkdir(const char *_pathname, mode_t mode)
{
    KERNEL_STRING(pathname, _pathname);
}
int sys_rmdir(const char *_pathname)
{
    KERNEL_STRING(pathname, _pathname);
}
int sys_dup(int oldfd)
{

}
int sys_dup2(int oldfd, int newfd)
{

}*/

/*static void dump_super(struct super_block *sb)
{
    if (sb) {
        printf("super block: 0x%08x\n\t"
            "magic: 0x%08x\n\t"
            "dev: %d\n\t"
            "inode map: %d - %d, %d blocks\n\t"
            "zone map: %d - %d, %d blocks\n\t"
            "inode table: %d - %d, %d blocks\n\t"
            "data: %d - ...\n\t",
            sb, sb->s_magic, sb->s_dev,
            sb->s_imap_start, sb->s_imap_start + sb->s_imap_count, sb->s_imap_count,
            sb->s_zmap_start, sb->s_zmap_start + sb->s_zmap_count, sb->s_zmap_count,
            sb->s_itable_start, sb->s_itable_start + sb->s_itable_count, sb->s_itable_count,
            sb->s_data_start);
    }
    else
        printf("super block: NULL POINTER!\n");
}*/

static void dump_inode(struct inode *inode)
{
    struct buffer_head *bh = NULL;
    int i, j;

    printf("inode [%d]: 0x%08x\n\t"
        "super block: 0x%08x\n\t"
        "size: %d\n\t"
        "mode: 0x%08x\n\t"
        "nlink: %d\n\t"
        "zone: %d %d %d ...\n\t",
        inode->i_inum, inode, inode->i_super,
        inode->i_size, inode->i_mode, inode->i_nlink,
        inode->i_zone[0], inode->i_zone[1], inode->i_zone[2]);

    for (j = 0; j < 3 && inode->i_zone[j]; ++j) {
        bh = bread(inode->i_super->s_dev, inode->i_zone[j]);
        for (i = 0; i < 32; ++i) {
            printf("%02x", ((char *)bh->b_data)[i]);
        }
        printf("\n");
        brelease(bh);
    }
}
void fs_dump_cache(void)
{
    struct inode *inode;
    struct dentry *dentry;
    printf("inode list:\n\t");
    list_for_each_entry(inode, &inode_list, i_list) {
        printf("%d ", inode->i_inum);
    }
    printf("\ndentry list:\n\t");
    list_for_each_entry(dentry, &dentry_list, d_list) {
        printf("'%s' ", dentry->d_name.name);
    }
    printf("\n");
}

static void mkfs (void)
{
    struct buffer_head *bh = NULL;

    super.s_magic = SUPER_MAGIC;
    super.s_imap_start = 2;
    super.s_imap_count = 1;
    super.s_zmap_start = 3;
    super.s_zmap_count = 1;
    super.s_itable_start = 4;
    super.s_itable_count = 6;
    super.s_data_start = 10;

    super.s_dev = 3;

    /* write super block */
    write_super(&super);

    /* write inode bitmap */
    bh = bread(super.s_dev, super.s_imap_start);
    memset(bh->b_data, 0, BLOCK_SIZE);
    _set_n_bit(bh->b_data, 2); // null, '/'
    bwrite(bh);
    brelease(bh);

    /* write block bitmap */
    bh = bread(super.s_dev, super.s_zmap_start);
    memset(bh->b_data, 0, BLOCK_SIZE);
    _set_n_bit(bh->b_data, super.s_data_start);
    bwrite(bh);
    brelease(bh);

    /* write inode array */
    root_inode.i_inum = 1;
    root_inode.i_size = 0;
    root_inode.i_nlink = 2; /* '.', '..' */
    memset(root_inode.i_zone, 0, 9 * sizeof(sector_t));
    root_inode.i_super = &super;
    INIT_LIST_HEAD(&root_inode.i_dentry);
    root_inode.i_refcnt = 1;

    /* write '/' */
    write_inode(&root_inode);
    // dump_inode(&root_inode);
}

void fs_init (void)
{
    /* check if filesystem exists */
    super.s_magic = 0;
    read_super(3);
    if (super.s_magic == SUPER_MAGIC) {
        printf("mkfs(): filesystem already exists.\n");
    }
    else
        mkfs();
    // mkfs();

    /* init super block, root inode, root dentry */
    struct super_block *sb = read_super(3);
    assert(sb == &super);
    // dump_super(sb);

    read_inode(&root_inode, 1, sb);
    INIT_LIST_HEAD(&root_inode.i_list);
    INIT_LIST_HEAD(&root_inode.i_dentry);
    root_inode.i_refcnt = 0;

    root_dentry.d_parent = &root_dentry;
    root_dentry.d_name.name = "/";
    root_dentry.d_name.len = 1;
    INIT_LIST_HEAD(&root_dentry.d_alias);
    INIT_LIST_HEAD(&root_dentry.d_list);
    root_dentry.d_refcnt = 0;

    __dlink_inode(&root_dentry, &root_inode);
    dump_inode(&root_inode);

    /* add root inode & dentry to cache */
    list_add(&root_inode.i_list, &inode_list);
    list_add(&root_dentry.d_list, &dentry_list);
    fs_dump_cache();

    assert(root_dentry.d_inode == &root_inode);
    assert(root_dentry.d_inode->i_super == &super);

    /* init cache */
    inode_cache = kmem_cache_create("inode", sizeof(struct inode));
    dentry_cache = kmem_cache_create("dentry", sizeof(struct dentry));
    file_cache = kmem_cache_create("file", sizeof(struct file));
    dname_cache = kmem_cache_create("dname", MAX_DENTRY_LENGTH);
}
