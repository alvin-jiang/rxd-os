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

extern uint32 get_phy_page (void);
extern void free_phy_page (uint32 page_addr);
extern void share_vma (uint32 from, uint32 to, uint32 size);
// extern void map_addr_to_phy_page (uint32 addr,uint32 page_addr);

extern void do_no_page (uint32 err_code, uint32 addr);
extern void do_wp_page (uint32 err_code, uint32 addr);

extern void mem_stat (void);

#endif

