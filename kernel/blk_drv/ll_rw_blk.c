#include "blk.h"

#include "proc.h" /* current_task */
#include "assert.h"
#include "stddef.h"
#include "string.h"

struct bio_request request[NR_REQUEST];

struct blk_dev blk_dev[NR_BLK_DEV] = {
    { NULL, NULL, NULL },     /* no_dev */
    { NULL, NULL, NULL },     /* dev mem */
    { NULL, NULL, NULL },     /* dev fd */
    { hd_init, hd_handle_request, NULL }      /* dev hd */
};
/* TODO: open(mount)/close(unmout)/read/write/ioctl */

void add_request (const struct bio_request *req)
{
    /* write request can only fill 2/3 request at maximal */
    int begin = (req->cmd.rw == BIO_READ) ? 0 : (NR_REQUEST / 3);

    /* find an empty request slot */
    int i;
    for (i = begin; i < NR_REQUEST; ++i)
    {
        if (request[i].dev == -1) {
            // printf("BLK: new request %d\n", i);
            memcpy(&request[i], req, sizeof(struct bio_request));
            break;
        }
    }
    if (i == NR_REQUEST)
        panic("BLK: Too many requests.");

    klock();
    /* if device not busy, just call handle function */
    if (!blk_dev[request[i].dev].bd_request) {
        blk_dev[request[i].dev].bd_request = &request[i];
        kunlock();
        (blk_dev[request[i].dev].bd_req_handle)(
            blk_dev[request[i].dev].bd_request
        );
        blk_dev[request[i].dev].bd_request = NULL;
    }
    /* else, add to device request list */
    else {
        panic("BLK: We don't support wait request list now.");
        request[i].next = blk_dev[request[i].dev].bd_request;
        blk_dev[request[i].dev].bd_request = &request[i];
        kunlock();
    }
}

void ll_rw_block (dev_t dev, int rw, u32 sector, u32 count, char * buffer)
{
    assert(dev > 0 && dev < NR_BLK_DEV);
    assert(rw == BIO_READ || rw == BIO_WRITE);
    if (dev != BLK_DEV_HD)
        panic("BLK: Device not supported.");

    struct bio_request r = {{rw, sector, count, buffer}, dev, NULL, NULL};
    add_request(&r);
}

void blk_dev_init (void)
{
    printf("init block devices...\n");
    int i;
    for (i = 0; i < NR_REQUEST; ++i) {
        request[i].dev = -1;
        request[i].next = NULL;
    }

    for (i = 0; i < NR_BLK_DEV; ++i) {
        if (blk_dev[i].bd_init)
            (blk_dev[i].bd_init)();
    }
}
