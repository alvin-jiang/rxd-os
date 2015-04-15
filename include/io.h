/**
 *
 * @file: io.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-04-11
 *
 */

#ifndef __IO_H__
#define __IO_H__

#include "assert.h"

#define QUEUE_BUF_SZ 16//256
#define QUEUE_FULL(q) ((q).count == QUEUE_BUF_SZ)
#define QUEUE_EMPTY(q) (!((q).count))

#define QUEUE_INIT(q) do { \
    (q).head = (q).tail = (q).buffer; \
    (q).count = 0; \
    } while(0)

#define QUEUE_ENQ(q, v) do { \
    assert( (q).count < QUEUE_BUF_SZ ); \
    if ((q).tail - (q).buffer >= QUEUE_BUF_SZ) \
        (q).tail -= QUEUE_BUF_SZ; \
    else ; \
    *((q).tail++) = (v); \
    ++(q).count; \
    } while(0)

#define QUEUE_DEQ(q, v) do { \
    assert( (q).count > 0 ); \
    (v) = *((q).head++); \
    if ((q).head - (q).buffer >= QUEUE_BUF_SZ) \
        (q).head -= QUEUE_BUF_SZ; \
    else ; \
    --(q).count; \
    } while(0)

struct tty_queue {
    int *head, *tail;
    int count;
    int buffer[QUEUE_BUF_SZ];
};

struct tty_struct {
    int kb_flag;
    struct tty_queue read_q, secondary;
};

extern int current_tty;

void io_init (void);

void tty_init (struct tty_struct * tty);
void tty_cook (struct tty_struct * tty);
int tty_read (struct tty_struct * tty, char * buf, int n);
int tty_write (struct tty_struct * tty, char * buf, int n);
// void copy_to_cooked (struct tty_struct * tty)


#endif

