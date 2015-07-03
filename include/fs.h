/**
 *
 * @file: fs.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-05-04
 *
 */


#ifndef __FS_H__
#define __FS_H__

#include "type.h"
#include "stddef.h"
#include "list.h"

struct dentry;
struct inode;
struct file;

struct super_block {
    unsigned long   s_magic;
    sector_t        s_imap_start;
    sector_t        s_imap_count;
    sector_t        s_zmap_start;
    sector_t        s_zmap_count;
    sector_t        s_itable_start;
    sector_t        s_itable_count;
    sector_t        s_data_start;

    dev_t           s_dev;
};

// #define SUPER_BLOCK_SIZE ()

struct inode {
    unsigned long   i_inum;
    off_t           i_size;
    unsigned short  i_mode; /* -rwxrwxrwx */
    short           i_nlink;
    sector_t        i_zone[9]; /* 0-6: direct 7: indirect 8: double indirect */

    struct super_block * i_super;
    struct list_head i_list;
    struct list_head i_dentry;
    int             i_refcnt;
};

#define D_INODE_SIZE ((int)(((struct inode *)0)->i_super))

#define MAX_DENTRY_LENGTH 24

struct dir_entry {
    u32     inode;
    u16     rec_len;
    u8      name_len;
    u8      file_type;
    char    name[MAX_DENTRY_LENGTH];
};

#define FT_UNKNOWN  0
#define FT_REG_FILE 1
#define FT_DIR      2

// #define DIR_PAD 4
// #define DIR_ROUND (DIR_PAD - 1)
// #define DIR_REC_LEN(name_len) (((name_len) + 8 + DIR_ROUND) & ~DIR_ROUND)
#define DIR_REC_LEN (32)

struct qstr {
    unsigned int    len; /* not include '/' and '\0' */
    const char *name;
};

struct dentry {
    struct dentry * d_parent;

    struct qstr     d_name;
    struct inode *  d_inode;
    struct list_head d_alias;
    struct list_head d_list;

    int             d_refcnt;
};

struct file {
    unsigned int    f_flags;
    mode_t          f_mode;
    off_t           f_pos;
    int             f_refcnt;
    struct dentry * f_dentry;
};

/* checklist
    . basic function
    . reference count (usage, child-parent)
    . [dentry] management of qstr.name
    . dirty flag
    . NULL pattern
    . lock & synchronization
*/

/* inode & memory cache */
struct inode * create_inode(struct super_block *sb, unsigned short mode);
struct inode * get_inode(struct super_block *sb, unsigned long inum);
void put_inode(struct inode *inode);
// int destroy_inode(struct inode * inode);
/* inode & disk */
int read_inode(struct inode *inode, unsigned long inum, struct super_block *sb);
void write_inode(struct inode *inode);
/* inode content */
// int read(struct inode * inode, int pos, char * buf, int count);
// int write(struct inode * inode, int pos, const char * buf, int count);

/* dentry */
// create tree node
// int create_dentry(struct inode *dir, struct dentry *dentry, int mode);
struct dentry * create_dentry(struct dentry *parent, struct qstr *dname, struct inode *inode);
// int destroy_dentry(struct dentry *dentry);
// load tree node
// struct dentry * get_dentry(struct inode *dir, struct qstr *qname);
struct dentry * get_dentry(struct dentry *parent, struct qstr *dname);
// unload tree node
void put_dentry(struct dentry *dentry);

/* high-level inode operations */
// struct dentry * lookup(struct inode *dir, struct dentry *dentry/*, struct nameidata *nd*/);
// int permission(struct inode *inode, int mask);
// int link(struct dentry *old_dentry, struct inode *dir,
//     struct dentry *new_dentry);
// int unlink(struct inode *dir, struct dentry *dentry);
// int mkdir(struct inode *dir, struct dentry *dentry, int mode);
// int rmdir(struct inode *dir, struct dentry *dentry);
// int rename(struct inode *old_dir, struct dentry *old_dentry,
//     struct inode *new_dir, struct dentry *new_dentry);

/* system call */
int sys_open(const char *pathname, int flags, mode_t mode);
// int sys_creat(const char *pathname, mode_t mode);
int sys_close(int fd);
ssize_t sys_read(int fd, void *buf, size_t count);
ssize_t sys_write(int fd, const void *buf, size_t count);

#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3
off_t sys_lseek(int fd, off_t offset, int whence);

int sys_truncate(const char *path, off_t length);
int sys_link(const char *oldpath, const char *newpath);
// int sys_unlink(const char *pathname);
// int sys_mkdir(const char *pathname, mode_t mode);
// int sys_rmdir(const char *pathname);
// int sys_dup(int oldfd);
// int sys_dup2(int oldfd, int newfd);

extern void fs_init (void);
extern void fs_dump_cache(void);
extern void fs_sync(void);

/* flags */
#define O_CREAT     1 /* bit 1: create */
#define O_APPEND    2 /* bit 2: append/truncate */
#define O_TRUNC     0

#define O_RDONLY    4 /* bit 3: read */
#define O_WRONLY    8 /* bit 4: write */
#define O_RDWR      12

#define O_SYNC      16 /* bit 5: sync/async */
#define O_ASYNC     0

#define O_DIRECT /* direct I/O, no buffer */


#endif

