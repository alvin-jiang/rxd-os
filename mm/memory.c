/**
 *
 * @file: memory.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-27
 *
 */

#include "mm.h"

#include "type.h"
#include "assert.h"
#include "string.h"

#define LOW_MEM 0x100000
#define TOTAL_MEM (15 * 1024 * 1024)

#define NR_PAGES (TOTAL_MEM >> 12)
#define mmi(addr) (((addr) - LOW_MEM) >> 12)

#define PDE(vaddr) ((u32 *)0x0 + ((vaddr) >> 22))
#define PTE(vaddr) ((u32 *)(*(PDE(vaddr)) & 0xfffff000) + (((vaddr) >> 12) & 0x3ff))
#define PAGE_ADDR(vaddr) (addr_t)(*(PTE(vaddr)) & 0xfffff000)
#define PHYSICAL_ADDR(vaddr) (addr_t)(*(PTE(vaddr)) & 0xfffff000 + (vaddr) & 0xfff)

#define RESERVED_PAGE (100)

#define PAGE_DEFAULT 0x7

static addr_t HIGH_MEMORY = 0;
static unsigned char mem_map[NR_PAGES];

void mem_init (addr_t mem_start, addr_t mem_end)
{
    printf("init memory module...\n");
    assert( mem_start >= LOW_MEM && mem_start < mem_end
        && mem_end < (LOW_MEM + TOTAL_MEM));
    HIGH_MEMORY = mem_end;

    int i, j;
    for (i = 0, j = NR_PAGES; i < j; ++i)
        mem_map[i] = RESERVED_PAGE;

    for (i = mmi(mem_start), j = mmi(mem_end);
        i <= j; ++i)
        mem_map[i] = 0;
    mem_stat();  
}

void mem_stat (void)
{
    int i, free_pages = 0;
    for (i = 0; i < NR_PAGES; ++i)
        if (!mem_map[i])
            ++free_pages;
    printf("[mem stat] %d free pages of %d\n", free_pages, NR_PAGES);
}

unsigned long alloc_pages(unsigned int order)
{
    unsigned int require = 1 << order;
    unsigned int accum = 0;

    int i;
    for (i = 0; i < NR_PAGES; ++i) {
        if (!mem_map[i])
            ++accum;
        else
            accum = 0;

        if (accum == require) {
            break;
        }
    }
    if (i == NR_PAGES) {
        panic("out of memory");
    }
    else {
        for (; accum > 0; --accum, --i) {
            assert(mem_map[i] == 0);
            ++(mem_map[i]);
        }
        ++i;
    }

    unsigned long page = LOW_MEM + (i << 12);
    assert(page >= LOW_MEM && page < HIGH_MEMORY);
    printf("mm: alloc %d pages, 0x%x\n", require, page);
    return page;
}
unsigned long alloc_page(void)
{
    return alloc_pages(0);
}

unsigned long alloc_zeroed_page(void)
{
    unsigned long addr = alloc_pages(0);
    memset((void *)addr, 0, PAGE_SIZE);
    return addr;
}

void free_pages(unsigned long addr, unsigned int order)
{
    unsigned int require = 1 << order;
    int start = mmi(addr);
    assert(start >= 0 && (start + require) < NR_PAGES);
    for (; require > 0; --require, ++start) {
        --mem_map[start];
        assert( mem_map[start] >= 0);
    }
    printf("mm: free %d pages, 0x%x\n", (1 << order), addr);
}

void free_page(unsigned long addr)
{
    free_pages(addr, 0);
}

// assume size == 4MB
void share_vma (addr_t from, addr_t to, addr_t size)
{
    assert( ((from & 0x003fffff) == 0) &&
        ((to & 0x003fffff) == 0) );
    assert( from > to ?
        (from - to >= size) : (to - from >= size) );

    printf("share_vma: %x -> %x [%x]\n", from, to, size);
    u32 * from_pde = PDE(from);
    u32 * to_pde = PDE(to);
    printf("from_pde: %x to_pde: %x\n", from_pde, to_pde);
    // alloc page table
    assert( *to_pde == 0 );
    // addr_t pt_addr = alloc_page();
    *to_pde = (alloc_page() & 0xfffff000) | PAGE_DEFAULT;
    printf("          -> to_pde: %x\n", to_pde);
    // while(1);

    // copy page table with write-protect enabled
    int i;
    u32 * from_page = (u32 *)(*from_pde & 0xfffff000);
    u32 * to_page = (u32 *)(*to_pde & 0xfffff000);
    for (i = 0; i < 1024; ++i) {
        *(to_page++) = (*(from_page++) & 0xfffffffd);
        // *(from_page++) = (*(to_page++) &= 0xfffffffd);
        // assert( mem_map[mmi(*to_page)] == 1 );
        // ++mem_map[mmi(*to_page)];
    }
    // TODO: for simplicity, we share video mem for now,
    // so we can use printk in user process
    *((u32 *)(*to_pde & 0xfffff000) + 144) |= 2;
}

void do_no_page (u32 err_code, addr_t addr)
{
    printf("PF: [No Page] code: %d addr: 0x%x\n", err_code, addr);
    // set PDE
    u32 * p_pde = PDE(addr);
    addr_t pt_addr = 0;
    if ( !(*p_pde & 0x1) ) {
        pt_addr = alloc_page();
        memset((void *)pt_addr, 0, PAGE_SIZE);
        *p_pde = (pt_addr & 0xfffff000) | PAGE_DEFAULT;
    }
    else
        pt_addr = (*p_pde & 0xfffff000);
    assert( pt_addr != 0 );
    // set PTE
    u32 * p_pte = (u32 *)pt_addr + ((addr >> 12) & 0x3ff);
    assert( *p_pte == 0 );
    addr_t pg_addr = alloc_page();
    *p_pte = (pg_addr & 0xfffff000) | PAGE_DEFAULT;
}

void do_wp_page (u32 err_code, addr_t addr)
{
    addr_t paddr = PAGE_ADDR(addr);
    printf("PF: [WP Page] code: %d addr: 0x%x paddr: 0x%x\n",
        err_code, addr, paddr);
    
    if (paddr < LOW_MEM) {
        // just remove wp
        u32 * p_pte = PTE(addr);

        addr_t pg_addr = alloc_page();
        memcpy((void *)pg_addr, (void *)(*p_pte & 0xfffff000), PAGE_SIZE);
        printf("!! kernel page, COW: %x -> %x\n", (*p_pte & 0xfffff000), pg_addr);
        *p_pte = (pg_addr & 0xfffff000) | PAGE_DEFAULT;
        return;
    }

    printk("OH~~~no~wp-page\n");
    while(1);
    
    assert( mem_map[mmi(paddr)] != 0 );
    if (mem_map[mmi(paddr)] == 1) {
        // just remove wp
        *(PTE(addr)) |= 0x2;
    }
    else {
        assert( mem_map[mmi(paddr)] >= 2);
        if (addr >= LOW_MEM) { // copy on write
            u32 * p_pte = PTE(addr);

            addr_t pg_addr = alloc_page();
            memcpy((void *)pg_addr, (void *)(*p_pte & 0xfffff000), PAGE_SIZE);
            *p_pte = (pg_addr & 0xfffff000) | PAGE_DEFAULT;
            --mem_map[mmi(addr)];
        }
        else {
            ;
        }
    }
}

