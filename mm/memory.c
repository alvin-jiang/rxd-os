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
static uint32 HIGH_MEMORY = 0;
#define TOTAL_MEM (15 * 1024 * 1024)

#define NR_PAGES (TOTAL_MEM >> 12)
#define mmi(addr) (((addr) - LOW_MEM) >> 12)

#define PDE(vaddr) ((uint32 *)0x0 + ((vaddr) >> 22))
#define PTE(vaddr) ((uint32 *)(*(PDE(vaddr)) & 0xfffff000) + (((vaddr) >> 12) & 0x3ff))
#define PAGE_ADDR(vaddr) (uint32)(*(PTE(vaddr)) & 0xfffff000)
#define PHYSICAL_ADDR(vaddr) (uint32)(*(PTE(vaddr)) & 0xfffff000 + (vaddr) & 0xfff)

#define RESERVED_PAGE (100)

#define PAGE_DEFAULT 0x7

static unsigned char mem_map[NR_PAGES];

void mem_init (uint32 mem_start, uint32 mem_end)
{
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

uint32 get_phy_page (void)
{
    int i;
    for (i = 0; i < NR_PAGES; ++i) {
        if (!mem_map[i]) {
            ++(mem_map[i]);
            break;
        }
    }

    if (i == NR_PAGES) {
        // oom();
        assert(0);
    }

    // assert( address is 4 KB aligned );
    printf("malloc: 0x%x\n", LOW_MEM + (i << 12));
    return LOW_MEM + (i << 12);
}

void free_phy_page (uint32 page_addr)
{
    assert( mem_map[mmi(page_addr)] > 0 );

    --mem_map[mmi(page_addr)];
}

// assume size == 4MB
void share_vma (uint32 from, uint32 to, uint32 size)
{
    assert( ((from & 0x003fffff) == 0) &&
        ((to & 0x003fffff) == 0) );
    assert( from > to ?
        (from - to >= size) : (to - from >= size) );

    printf("share_vma: %x -> %x [%x]\n", from, to, size);
    uint32 * from_pde = PDE(from);
    uint32 * to_pde = PDE(to);
    printf("from_pde: %x to_pde: %x\n", from_pde, to_pde);
    // alloc page table
    assert( *to_pde == 0 );
    // uint32 pt_addr = get_phy_page();
    *to_pde = (get_phy_page() & 0xfffff000) | PAGE_DEFAULT;
    printf("          -> to_pde: %x\n", to_pde);
    // while(1);

    // copy page table with write-protect enabled
    int i;
    uint32 * from_page = (uint32 *)(*from_pde & 0xfffff000);
    uint32 * to_page = (uint32 *)(*to_pde & 0xfffff000);
    for (i = 0; i < 1024; ++i) {
        *(to_page++) = (*(from_page++) & 0xfffffffd);
        // *(from_page++) = (*(to_page++) &= 0xfffffffd);
        // assert( mem_map[mmi(*to_page)] == 1 );
        // ++mem_map[mmi(*to_page)];
    }
    // video mem must share!!!
    *((uint32 *)(*to_pde & 0xfffff000) + 144) |= 2;
}

void do_no_page (uint32 err_code, uint32 addr)
{
    printf("PF: [No Page] code: %d addr: 0x%x\n", err_code, addr);
    // set PDE
    uint32 * p_pde = PDE(addr);
    uint32 pt_addr = 0;
    if ( !(*p_pde & 0x1) ) {
        pt_addr = get_phy_page();
        memset((void *)pt_addr, 0, PAGE_SIZE);
        *p_pde = (pt_addr & 0xfffff000) | PAGE_DEFAULT;
    }
    else
        pt_addr = (*p_pde & 0xfffff000);
    assert( pt_addr != 0 );
    // set PTE
    uint32 * p_pte = (uint32 *)pt_addr + ((addr >> 12) & 0x3ff);
    assert( *p_pte == 0 );
    uint32 pg_addr = get_phy_page();
    *p_pte = (pg_addr & 0xfffff000) | PAGE_DEFAULT;
}

void do_wp_page (uint32 err_code, uint32 addr)
{
    uint32 paddr = PAGE_ADDR(addr);
    printf("PF: [WP Page] code: %d addr: 0x%x paddr: 0x%x\n",
        err_code, addr, paddr);
    
    if (paddr < LOW_MEM) {
        // just remove wp
        uint32 * p_pte = PTE(addr);

        uint32 pg_addr = get_phy_page();
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
            uint32 * p_pte = PTE(addr);

            uint32 pg_addr = get_phy_page();
            memcpy((void *)pg_addr, (void *)(*p_pte & 0xfffff000), PAGE_SIZE);
            *p_pte = (pg_addr & 0xfffff000) | PAGE_DEFAULT;
            --mem_map[mmi(addr)];
        }
        else {
            ;
        }
    }
}

