/**
 *
 * @file: printf.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-26
 *
 */

#include "stdio.h"
#include "type.h"

// return pos of string terminator ('\0')
int itoa ( unsigned int value, char * str, int base )
{
    char *p = str;
    if (!value) {
        *p++ = '0';
    } else {
        while (value) {
            *p = value % base;
            *p = (*p < 10) ? (*p + '0') : (*p - 10 + 'A');
            ++p;
            value /= base;
        }

        char *h = str, *t = (p - 1), c;
        do {
            c = *h;
            *h++ = *t;
            *t-- = c;
        } while (h < t);
    }
    return p - str;
}

int vsprintf (char * str, const char * format, va_list arg )
{
    int state = 0;

    const char *p = format;
    char *s = str;
    while (*p) {
        *s = *p++;
        switch (*s) {
        case '%':
            state = 1;
            break;
        case 'd':
        case 'x':
            if (state == 1) {
                s += itoa(*((unsigned int *)arg), s, (*p == 'd') ? 10 : 16);
                state = 0;
                arg += 4;
            } else
                ++s;
            break;
        default:
            ++s;
            break;
        }
    }
    *s++ = '\0';
    return s - str;
}

int sprintf ( char * str, const char * format, ... )
{
    va_list arg = (va_list)((char*)(&format) + 4);
    return vsprintf(str, format, arg);
}


int printf(const char *fmt, ...)
{
	char buf[256];
	int i = vsprintf(buf, fmt, (va_list)((char*)(&fmt) + 4));
	printk(buf);
	return i;
}

