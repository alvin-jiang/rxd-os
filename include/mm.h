/**
 *
 * @file: mm.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-27
 *
 */
#ifndef __MM_H__
#define __MM_H__

#include "type.h"

#define PAGE_SIZE 4096

extern void share_vma (addr_t from, addr_t to, addr_t size);
// extern void map_addr_to_phy_page (addr_t addr,addr_t page_addr);

extern void do_no_page (u32 err_code, addr_t addr);
extern void do_wp_page (u32 err_code, addr_t addr);

extern void mem_stat (void);
extern void mem_init (addr_t mem_start, addr_t mem_end);

unsigned long alloc_page(void);
unsigned long alloc_zeroed_page(void);
unsigned long alloc_pages(unsigned int order);
void free_pages(unsigned long addr, unsigned int order);
void free_page(unsigned long addr);

// struct page {};
// struct page * __alloc_page(int gfp_flags);
// struct page * __alloc_pages(int gfp_flags, unsigned int order);
// void __free_pages(struct page *page, unsigned int order);
// void * page_address(struct page *page);

/* slab */
#include "list.h"

struct kmem_cache;

typedef unsigned int kmem_bufctl_t;

struct slab {
    struct list_head list;
    struct kmem_cache *cache;

    unsigned char *data;

    /* free list */
    unsigned int inuse;
    kmem_bufctl_t free;
};

/* slab manager */
struct kmem_cache {
    struct list_head slabs_partial;
    struct list_head slabs_full;
    struct list_head slabs_free;

    unsigned int objsz;
    unsigned int objcnt;
    // unsigned int flags;
    void (*ctor)(void *);

    const char *name;
    struct list_head next;
};

struct kmem_cache * kmem_cache_create(const char *name, size_t objsize);
struct kmem_cache * kmem_cache_createx(const char *name, size_t objsize, void (*ctor)(void *));
int kmem_cache_destroy(struct kmem_cache *cachep);
void * kmem_cache_alloc(struct kmem_cache *cachep);
void kmem_cache_free(struct kmem_cache *cachep, void *objp);

void * kmalloc(size_t size);
void kfree(void *ptr);

void cache_init(void);

#endif

