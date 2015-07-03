#include "mm.h"
#include "list.h"
#include "type.h"
#include "assert.h"

#define __align(addr, order) \
    ( (unsigned long)(addr) & ~((1 << (order))-1) )

#define BUFCTL_END  0xffffFFFF
// #define BUFCTL_FREE 0xffffFFFE

#define ADDR_TO_SLAB(addr) \
    ( (struct slab *)( ((unsigned long)(addr) & ~0xfff) + PAGE_SIZE - sizeof(struct slab) ) )

#define slab_bufctl(objnr, slab) \
    ( ((kmem_bufctl_t *)(slab))[-1 - (objnr)] )
#define slab_obj(objnr, slab) \
    ( (void *)(slab->data + (objnr) * slab->cache->objsz) )

/* list of all caches */
static struct list_head cache_chain = LIST_HEAD_INIT(cache_chain);

/* cache of 'struct kmem_cache' */
static struct kmem_cache cache_cache = {
    LIST_HEAD_INIT(cache_cache.slabs_partial),
    LIST_HEAD_INIT(cache_cache.slabs_full),
    LIST_HEAD_INIT(cache_cache.slabs_free),
    sizeof(struct kmem_cache),
    (PAGE_SIZE - sizeof(struct slab)) / (sizeof(struct kmem_cache) + sizeof(kmem_bufctl_t)),
    NULL,
    "kmem_cache",
    LIST_HEAD_INIT(cache_cache.next)
};

/* for generic malloc/free */
enum generic_size {_4B, _8B, _16B, _32B, _64B, _128B, GENERIC_SIZE_COUNT};
static struct kmem_cache * generic_cachep[GENERIC_SIZE_COUNT];

static inline void __check_cache(struct kmem_cache *cachep)
{
    assert(cachep);
    assert( (cachep == &cache_cache) ||
        (ADDR_TO_SLAB(cachep)->cache == &cache_cache) );
    assert(cachep->objcnt ==
        ((PAGE_SIZE - sizeof(struct slab)) / (cachep->objsz + sizeof(kmem_bufctl_t))));
}
/* test if slab is valid */
static void __check_slab(struct slab *slab)
{
    /* address */
    assert(slab);
    assert( !((unsigned long)(slab + 1) & 0xfff) );

    /* slab */
    assert( slab->data && !((unsigned long)(slab->data) & 0xfff) );

    /* slab & cache */
    struct kmem_cache *cachep = slab->cache;
    __check_cache(cachep);

    unsigned int freecnt = 0;
    kmem_bufctl_t next = slab->free;
    while (next != BUFCTL_END) {
        ++freecnt;
        next = slab_bufctl(next, slab);
    }
    assert((freecnt + slab->inuse) == cachep->objcnt);
}

static struct slab * _create_slab(struct kmem_cache *cachep)
{
    __check_cache(cachep);

    unsigned char *page;
    struct slab *slab;
    kmem_bufctl_t *bufctl;
    void *obj;
    int i;

    /* get page */
    page = (unsigned char *)alloc_zeroed_page();

    slab = ADDR_TO_SLAB(page);
    INIT_LIST_HEAD(&slab->list);
    slab->cache = cachep;
    slab->data = page;
    slab->inuse = 0;
    slab->free = 0;

    bufctl = (kmem_bufctl_t *)slab;
    if (cachep->ctor) {
        obj = slab->data;
        for (i = 0; i < cachep->objcnt; ++i) {
            *(--bufctl) = i + 1;
            cachep->ctor(obj);
            obj = (void *)((char *)obj + cachep->objsz);
        }
    }
    else {
        for (i = 0; i < cachep->objcnt; ++i) {
            *(--bufctl) = i + 1;
        }
    }
    *bufctl = BUFCTL_END;

    /* add slab to cache */
    list_add(&slab->list, &cachep->slabs_free);

    __check_slab(slab);
    return slab;
}
static void _destroy_slab(struct slab *slab)
{
    assert(slab->inuse == 0);
    __check_slab(slab);

    /* remove slab from cache */
    list_del(&slab->list);

    /* release page */
    free_page((unsigned long)slab->data);
}

static void * _slab_alloc(struct slab *slab)
{
    assert(slab->free != BUFCTL_END);
    __check_slab(slab);

    /* remove obj from free list */
    int objnr = slab->free;
    slab->free = slab_bufctl(slab->free, slab);

    ++(slab->inuse);

    /* fix slab position in cache */
    list_del(&slab->list);
    if (slab->free == BUFCTL_END)
        list_add(&slab->list, &slab->cache->slabs_full);
    else
        list_add(&slab->list, &slab->cache->slabs_partial);
    return slab_obj(objnr, slab);
}

static void _slab_free(void *objp)
{
    struct slab *slab;
    int objnr;

    slab = ADDR_TO_SLAB(objp);
    __check_slab(slab);
    objnr = (objp - (void *)slab->data) / slab->cache->objsz;

    /* add obj to free list */
    slab_bufctl(objnr, slab) = slab->free;
    slab->free = objnr;

    --(slab->inuse);

    /* fix slab position in cache */
    list_del(&slab->list);
    if (slab->inuse == 0)        
        list_add(&slab->list, &slab->cache->slabs_free);
    else
        list_add(&slab->list, &slab->cache->slabs_partial);
}

void * kmem_cache_alloc(struct kmem_cache *cachep)
{
    __check_cache(cachep);

    struct slab *slab;

    // printf("cache '%s': alloc\n", cachep->name);

    /* find a non-full slab */
    if (list_empty(&cachep->slabs_partial)) {
        if (list_empty(&cachep->slabs_free))
            slab = _create_slab(cachep);
        else
            slab = list_entry(cachep->slabs_free.next, struct slab, list);
    }
    else
        slab = list_entry(cachep->slabs_partial.next, struct slab, list);

    return _slab_alloc(slab);
}

void kmem_cache_free(struct kmem_cache *cachep, void *objp)
{
    assert(cachep && objp);

    _slab_free(objp);
}

struct kmem_cache * kmem_cache_createx(const char *name, size_t objsize, void (*ctor)(void *))
{
    assert(name && objsize > 0);

    struct kmem_cache *cachep = kmem_cache_alloc(&cache_cache);

    INIT_LIST_HEAD(&(cachep->slabs_partial)); assert(list_empty(&cachep->slabs_partial));
    INIT_LIST_HEAD(&(cachep->slabs_full)); assert(list_empty(&cachep->slabs_full));
    INIT_LIST_HEAD(&(cachep->slabs_free)); assert(list_empty(&cachep->slabs_free));
    cachep->objsz = __align(objsize, 2); // 4 byte alignment
    cachep->objcnt = (PAGE_SIZE - sizeof(struct slab)) / (cachep->objsz + sizeof(kmem_bufctl_t));
    cachep->ctor = ctor;

    cachep->name = name;
    list_add(&cachep->next, &cache_chain);

    return cachep;
}
struct kmem_cache * kmem_cache_create(const char *name, size_t objsize)
{
    return kmem_cache_createx(name, objsize, NULL);
}

int kmem_cache_destroy(struct kmem_cache *cachep)
{
    assert(cachep);

    struct slab *slab;

    if (!list_empty(&cachep->slabs_partial) ||
        !list_empty(&cachep->slabs_full)) {
        panic("can't destroy cache '%s', it's in use.\n");
    }

    while (!list_empty(&cachep->slabs_free)) {
        slab = list_entry(cachep->slabs_free.next, struct slab, list);
        _destroy_slab(slab);
    }

    list_del(&cachep->next);
    return 0;
}

void * kmalloc(size_t size)
{
    assert(size > 0 && size <= 128);

    struct kmem_cache *cachep = NULL;

    if (size <= 32) {
        if (size <= 8) {
            if (size <= 4)
                cachep = generic_cachep[_4B];
            else
                cachep = generic_cachep[_8B];
        }
        else {
            if (size <= 16)
                cachep = generic_cachep[_16B];
            else
                cachep = generic_cachep[_32B];
        }
    }
    else {
        if (size <= 64)
            cachep = generic_cachep[_64B];
        else
            cachep = generic_cachep[_128B];
    }

    void *objp = kmem_cache_alloc(cachep);
    printf("kmalloc: 0x%08x %d bytes\n", objp, cachep->objsz);
    return objp;
}

void kfree(void *ptr)
{
    assert(ptr);

    struct slab *slab = ADDR_TO_SLAB(ptr);
    __check_slab(slab);

    kmem_cache_free(slab->cache, ptr);
    printf("kfree:   0x%08x %d bytes\n", ptr, slab->cache->objsz);
}

static inline void cache_stat(void)
{
    struct kmem_cache *cachep;
    printf("[cache stat]\n");
    list_for_each_entry(cachep, &cache_chain, next) {
        printf("\t[0x%08x] '%s': size=%d\tcount=%d\n",
            cachep, cachep->name, cachep->objsz, cachep->objcnt);
    }
}

void cache_init(void)
{
    printf("init cache module...\n");
    /* init cache of 'struct kmem_cache' */
    list_add(&cache_cache.next, &cache_chain);

    /* init generic caches */
    generic_cachep[_4B] = kmem_cache_create("4 Byte", 4);
    generic_cachep[_8B] = kmem_cache_create("8 Byte", 8);
    generic_cachep[_16B] = kmem_cache_create("16 Byte", 16);
    generic_cachep[_32B] = kmem_cache_create("32 Byte", 32);
    generic_cachep[_64B] = kmem_cache_create("64 Byte", 64);
    generic_cachep[_128B] = kmem_cache_create("128 Byte", 128);
    cache_stat();
}

