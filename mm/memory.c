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
static DWORD HIGH_MEMORY = 0;
#define TOTAL_MEM (15 * 1024 * 1024)

#define NR_PAGES (TOTAL_MEM >> 12)
#define mmi(addr) (((addr) - LOW_MEM) >> 12)

#define PDE(vaddr) ((DWORD *)0x0 + ((vaddr) >> 22))
#define PTE(vaddr) ((DWORD *)(*(PDE(vaddr)) & 0xfffff000) + (((vaddr) >> 12) & 0x3ff))
#define PAGE_ADDR(vaddr) (DWORD)(*(PTE(vaddr)) & 0xfffff000)
#define PHYSICAL_ADDR(vaddr) (DWORD)(*(PTE(vaddr)) & 0xfffff000 + (vaddr) & 0xfff)

#define RESERVED_PAGE (100)

#define PAGE_DEFAULT 0x7

static unsigned char mem_map[NR_PAGES];

void mem_init (DWORD mem_start, DWORD mem_end)
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

DWORD get_phy_page (void)
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

void free_phy_page (DWORD page_addr)
{
    assert( mem_map[mmi(page_addr)] > 0 );

    --mem_map[mmi(page_addr)];
}

// assume size == 4MB
void share_vma (DWORD from, DWORD to, DWORD size)
{
    assert( ((from & 0x003fffff) == 0) &&
        ((to & 0x003fffff) == 0) );
    assert( from > to ?
        (from - to >= size) : (to - from >= size) );

    printf("share_vma: %x -> %x [%x]\n", from, to, size);
    DWORD * from_pde = PDE(from);
    DWORD * to_pde = PDE(to);
    printf("from_pde: %x to_pde: %x\n", from_pde, to_pde);
    // alloc page table
    assert( *to_pde == 0 );
    // DWORD pt_addr = get_phy_page();
    *to_pde = (get_phy_page() & 0xfffff000) | PAGE_DEFAULT;
    printf("          -> to_pde: %x\n", to_pde);
    // while(1);

    // copy page table with write-protect enabled
    int i;
    DWORD * from_page = (DWORD *)(*from_pde & 0xfffff000);
    DWORD * to_page = (DWORD *)(*to_pde & 0xfffff000);
    for (i = 0; i < 1024; ++i) {
        *(to_page++) = (*(from_page++) & 0xfffffffd);
        // *(from_page++) = (*(to_page++) &= 0xfffffffd);
        // assert( mem_map[mmi(*to_page)] == 1 );
        // ++mem_map[mmi(*to_page)];
    }
    // video mem must share!!!
    *((DWORD *)(*to_pde & 0xfffff000) + 144) |= 2;
}

void do_no_page (DWORD err_code, DWORD addr)
{
    printf("PF: [No Page] code: %d addr: 0x%x\n", err_code, addr);
    // set PDE
    DWORD * p_pde = PDE(addr);
    DWORD pt_addr = 0;
    if ( !(*p_pde & 0x1) ) {
        pt_addr = get_phy_page();
        memset((void *)pt_addr, 0, PAGE_SIZE);
        *p_pde = (pt_addr & 0xfffff000) | PAGE_DEFAULT;
    }
    else
        pt_addr = (*p_pde & 0xfffff000);
    assert( pt_addr != 0 );
    // set PTE
    DWORD * p_pte = (DWORD *)pt_addr + ((addr >> 12) & 0x3ff);
    assert( *p_pte == 0 );
    DWORD pg_addr = get_phy_page();
    *p_pte = (pg_addr & 0xfffff000) | PAGE_DEFAULT;
}

void do_wp_page (DWORD err_code, DWORD addr)
{
    DWORD paddr = PAGE_ADDR(addr);
    printf("PF: [WP Page] code: %d addr: 0x%x paddr: 0x%x\n",
        err_code, addr, paddr);
    
    if (paddr < LOW_MEM) {
        // just remove wp
        DWORD * p_pte = PTE(addr);

        DWORD pg_addr = get_phy_page();
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
            DWORD * p_pte = PTE(addr);

            DWORD pg_addr = get_phy_page();
            memcpy((void *)pg_addr, (void *)(*p_pte & 0xfffff000), PAGE_SIZE);
            *p_pte = (pg_addr & 0xfffff000) | PAGE_DEFAULT;
            --mem_map[mmi(addr)];
        }
        else {
            ;
        }
    }
}

