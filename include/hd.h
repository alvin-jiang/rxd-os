
#include "type.h"

typedef struct hd_partition {
    uint8    boot_ind;
    uint8    start_head;
    uint8    start_sector;
    uint8    start_cyl;
    uint8    sys_id;
    uint8    end_head; 
    uint8    end_sector;
    uint8    end_cyl;
    uint32   start_sect; 
    uint32   nr_sects;
} hd_partition;

extern void hd_init ();
// extern int hd_rdwt (int start, char * buffer, int count);
extern int hd_rdwt (int rw, int start_sect, int sect_cnt, void * buffer);
extern void on_hd_interrupt (int int_nr);

#define SECTOR_SIZE 512
#define READ    0
#define WRITE   1
