/**
 *
 * @file: io.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-04-11
 *
 */

#include "io.h"

#include "head.h"
#include "keyboard.h"

extern void scroll_screen(int line);

int current_tty;
static struct tty_struct tty_table[1] = {
    {0, {0, 0, 0, {0},}, {0, 0, 0, {0},},},
};

void on_keyboard_interrupt (int int_nr)
{
    unsigned char scan_code = inb(0x60);
    struct tty_struct *tty = &(tty_table[current_tty]);
    struct tty_queue *q = &(tty->read_q);
    // printf("<0x%x>", scan_code);
    if (!QUEUE_FULL(*q)) {
        QUEUE_ENQ(*q, scan_code);
        tty_cook(tty);
        while (!QUEUE_EMPTY(tty->secondary)) {
            QUEUE_DEQ(tty->secondary, scan_code);
            // printf("%c", scan_code);
            switch(scan_code) {
                case 'j':
                case 'J':
                    scroll_screen(3);
                    break;
                case 'k':
                case 'K':
                    scroll_screen(-3);
                    break;
                default:
                    break;
            }
            // printf("ECHO: %c 0x%x %c\n", scan_code, scan_code,
            //     (tty->kb_flag & KB_F_SHIFT) ? 'S' : ' ');
        }
    }
    else
        panic("keyboard buffer is full");
}

void tty_init (struct tty_struct * tty)
{
    QUEUE_INIT(tty->read_q);
    QUEUE_INIT(tty->secondary);
}


// keyboard stuffs
int isprint (char c)
{
    return (c >= 0x20 && c <= 0x7e);
}

typedef enum {
    CS_ERROR,
    CS_NORMAL,
    CS_E0,
    CS_DONE,
} cook_state;

// no error toleration now
// if error, just panic
cook_state cook (int scan_code, cook_state cstate, int * kb_flag,
    int * count, char * out)
{
    assert( !(scan_code & 0xffffff00) && *count == 0 );

    char c;
    switch(cstate) {
    case CS_NORMAL:
        if (scan_code == 0xe0)
            cstate = CS_E0;
        else if (kb_keycode(scan_code) == KEY_CTRL_L) {
            // printf("Ctrl %s\n", (scan_code & 0x80) ? "up" : "down");
            *kb_flag = (scan_code & 0x80) ?
                (*kb_flag & ~(KB_F_CTRL)) : (*kb_flag | KB_F_CTRL);
            cstate = CS_DONE;
        }
        else if (kb_keycode(scan_code) == KEY_SHIFT_L) {
            // printf("Shift %s\n", (scan_code & 0x80) ? "up" : "down");
            *kb_flag = (scan_code & 0x80) ?
                 (*kb_flag & ~(KB_F_SHIFT)) : (*kb_flag | KB_F_SHIFT);
            cstate = CS_DONE;
        }
        else if (kb_keycode(scan_code) == KEY_ALT_L) {
            // printf("Alt %s\n", (scan_code & 0x80) ? "up" : "down");
            *kb_flag = (scan_code & 0x80) ?
                (*kb_flag & ~(KB_F_ALT)) : (*kb_flag | KB_F_ALT);
            cstate = CS_DONE;
        }
        else if (!(scan_code & 0x80)) {
            c = kb_ascii(scan_code, *kb_flag);
            if (isprint(c)) {
                *count = 1;
                *out = c;
                cstate = CS_DONE;
            }
            else {
                // printf("Non-printable <0x%x> %x\n", scan_code, c);
                cstate = CS_DONE;
            }
        }
        else {
            // printf("Non-printable <0x%x> %x\n", scan_code, c);
            cstate = CS_DONE;
        }
        break;
    case CS_E0:
        printf("[Warning] we can't deal with E0 case now.\n");
        cstate = CS_DONE;
        break;
    case CS_DONE:
        panic("keyboard: bad scan code sequence");
    default:
        panic("keyboard: unknown cook state");
    }

    return cstate;
}

void tty_cook (struct tty_struct * tty)
{
    int scan_code;
    int *save_head, save_count;

    cook_state state;
    int filter_cnt = 0;
    char filter_buf[5], *p;

    while (!QUEUE_FULL(tty->secondary) && !QUEUE_EMPTY(tty->read_q)) {
        // queue save
        save_head = tty->read_q.head;
        save_count = tty->read_q.count;
        QUEUE_DEQ(tty->read_q, scan_code);
        state = CS_NORMAL;
        filter_cnt = 0;
        while ((state = cook(scan_code, state,
            &(tty->kb_flag), &filter_cnt, filter_buf)) != CS_DONE) {
            
            if (QUEUE_EMPTY(tty->read_q)) {
                // queue restore
                tty->read_q.head = save_head;
                tty->read_q.count = save_count;
                printf("[WARNING] uncomplete cook\n");
                return;
            }
            QUEUE_DEQ(tty->read_q, scan_code);
        }
        // copy result to secondary
        if ((QUEUE_BUF_SZ - tty->secondary.count) < filter_cnt) {
            tty->read_q.head = save_head;
            tty->read_q.count = save_count;
            printf("[WARNING] secondary queue too small\n");
            return;
        }
        else {
            p = filter_buf;
            while (--filter_cnt >= 0) {
                QUEUE_ENQ(tty->secondary, *(p++));
            }
        }
    }
}

int tty_read (struct tty_struct * tty, char * buf, int n)
{
    return 0;
}
int tty_write (struct tty_struct * tty, char * buf, int n)
{
    return 0;
}

void io_init (void)
{
    // int i;

    // init tty
    current_tty = 0;
    tty_init(&tty_table[0]);

    // init keyboard
    set_irq_handler(IRQ1_KEYBOARD, on_keyboard_interrupt);
    enable_irq(IRQ1_KEYBOARD);
}

