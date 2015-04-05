/**
 *
 * @file: printf.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-26
 *
 */

#include "stdio.h"

#include "stdarg.h"
#include "assert.h"

#include "string.h"

#define PRINT_BUF (80)

// return pos of string terminator ('\0')
int itoa (unsigned int value, char * str, int base)
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

int vsprintf (char * str, const char * format, va_list args)
{
    int state = 0;

    const char *ap;
    
    const char *p = format;
    char *s = str;
    while (*p) {
        *s = *p++;
        switch (*s) {
        case '%':
            state = 1;
            break;
        case 's':
            if (state == 1) {
                ap = va_arg(args, const char *);
                assert( strlen(ap) <= 80 );
                strcpy(s, ap);
                s += strlen(ap);
                state = 0;
            } else
                ++s;
            break;
        case 'd':
        case 'x':
            if (state == 1) {
                s += itoa(va_arg(args, unsigned int), s, (*s == 'd') ? 10 : 16);
                state = 0;
            } else
                ++s;
            break;
        default:
            ++s;
            break;
        }
        assert( (s - str) < PRINT_BUF );
    }
    *s++ = '\0';
    return s - str;
}

int sprintf (char * str, const char * format, ...)
{
    va_list args;
    return vsprintf(str, format, va_start(args, format));
}

int printf (const char *format, ...)
{
	char buf[PRINT_BUF];

    va_list args;
	int i = vsprintf(buf, format, va_start(args, format));
	puts(buf);
	return i;
}

