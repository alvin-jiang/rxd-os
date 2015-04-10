/**
 *
 * @file: string.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-04-09
 *
 */

#include "string.h"

#include "assert.h"

/* operation size */
#define _OP4(v, p)      unsigned int * v = (unsigned int *)p;
#define _OP4C(v, p)     const unsigned int * v = (unsigned int *)p;
#define _OP1(v, p)      unsigned char * v = (unsigned char *)p;
#define _OP1C(v, p)     const unsigned char * v = (unsigned char *)p;
/* default operation size */
#define _OP(v, p)       _OP1(v, p)
#define _OPC(v, p)      _OP1C(v, p)

void * memcpy (void * _dst, const void * _src, size_t num)
{
    _OPC(src, _src)
    _OP(dst, _dst)

    // no overlay
    assert( (dst + num) <= src || dst >= (src + num) );
    if (!num || dst == src)
        return dst;

    do {
        *(dst++) = *(src++);
    } while (--num);

    return (void *)(dst - num);
}

void * memmove (void * _dst, const void * _src, size_t num)
{
    _OPC(src, _src)
    _OP(dst, _dst)

    if (!num || dst == src)
        return (void *)dst;

    if (dst > src) {
        dst += num;
        src += num;
        do {
            *(--dst) = *(--src);
        } while (--num);
        return (void *)dst;
    }
    else
        return memcpy(dst, src, num);
}

void * memset (void * _ptr, int value, size_t num)
{
    _OP(ptr, _ptr)

    if (!num)
        return ptr;

    do {
        *(ptr++) = (unsigned char)(value & 0xff);
    } while (--num);

    return (void *)(ptr - num);
}

char * strcpy (char * dst, const char * src)
{
    char *p = dst;
    while (*src != '\0') {
        *(p++) = *(src++);
    }
    *p = '\0';
    return dst;
}

size_t strlen (const char * str)
{
    const char *p = str;
    while (*p != '\0')
        ++p;
    return (size_t)(p - str);
}
