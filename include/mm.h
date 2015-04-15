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

extern DWORD get_phy_page (void);
extern void free_phy_page (DWORD page_addr);
extern void share_vma (DWORD from, DWORD to, DWORD size);
// extern void map_addr_to_phy_page (DWORD addr,DWORD page_addr);

extern void do_no_page (DWORD err_code, DWORD addr);
extern void do_wp_page (DWORD err_code, DWORD addr);

extern void mem_stat (void);

#endif

