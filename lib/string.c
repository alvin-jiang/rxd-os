/**
 *
 * @file: string.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-04-09
 *
 */

#include "string.h"
#include "stddef.h"
#include "assert.h"

/* operation size */
#define __u8p(cp, vp) unsigned char * cp = (unsigned char *)vp
#define __cu8p(cp, vp) const unsigned char * cp = (const unsigned char *)vp
#define __check_not_null(p) assert(p != NULL)

void * memcpy (void * _dst, const void * _src, size_t num)
{
    __check_not_null(_dst);
    __check_not_null(_src);
    __u8p(dst, _dst);
    __cu8p(src, _src);

    // overlay check
    assert( (dst + num) <= src || dst >= (src + num) );

    if (num && dst != src) {
        do {
            *(dst++) = *(src++);
        } while (--num);
    }

    return _dst;
}

void * memmove (void * _dst, const void * _src, size_t num)
{
    __check_not_null(_dst);
    __check_not_null(_src);
    __u8p(dst, _dst);
    __cu8p(src, _src);

    if (!num || dst == src)
        return (void *)dst;

    if (dst > src) {
        dst += num;
        src += num;
        do {
            *(--dst) = *(--src);
        } while (--num);
        return _dst;
    }
    else
        return memcpy(dst, src, num);
}

void * memset (void * _ptr, int _value, size_t num)
{
    __check_not_null(_ptr);
    __u8p(ptr, _ptr);

    if (!num)
        return _ptr;

    unsigned char value = (unsigned char)(_value & 0xff);
    do {
        *(ptr++) = value;
    } while (--num);

    return _ptr;
}

char * strcpy (char * dst, const char * src)
{
    __check_not_null(dst);
    __check_not_null(src);

    char *p = dst;
    while (*src != '\0') {
        *(p++) = *(src++);
    }
    *p = '\0';
    return dst;
}

char * strchr (const char * cstr, int character)
{
    __check_not_null(cstr);
    char *str = (char *)cstr;

    while (*str != '\0' && *str != character) {
        ++str;
    }
    return (*str == character) ? str : NULL;
}

char * strrchr ( const char * cstr, int character )
{
    __check_not_null(cstr);
    char *str = (char *)cstr;

    char *p = str;
    while (*str != '\0') {
        if (*str == character)
            p = str;
        ++str;
    }
    /* '\0' */
    if (*str == character)
        p = str;
    return (*p == character) ? p : NULL;
}

size_t strcspn ( const char * str1, const char * str2 )
{
    __check_not_null(str1);
    __check_not_null(str2);
    __cu8p(scan, str1);
    __cu8p(match, str2);

    /* build look-up table for faster search */
    unsigned char T[256];
    memset((void *)T, 0, sizeof(T));
    do {
        T[*match] = 1;
    } while (*(match++) != '\0');

    do {
        if (T[*scan])
            break;
    } while (*(scan++) != '\0');

    return (size_t)((const char *)scan - str1);
}

size_t strlen (const char * str)
{
    __check_not_null(str);

    const char *p = str;
    while (*p != '\0')
        ++p;
    return (size_t)(p - str);
}

char * strncpy ( char * dst, const char * src, size_t num )
{
    __check_not_null(dst);
    __check_not_null(src);

    char *p = dst;
    while (*src != '\0' && num--) {
        *(p++) = *(src++);
    }
    /* padding with '\0' */
    while (num--) {
        *(p++) = '\0';
    }
    return dst;
}

int strncmp ( const char * str1, const char * str2, size_t num )
{
    __check_not_null(str1);
    __check_not_null(str2);

    if (!num)
        return 0;

    while (num-- && *str1 == *str2 && *str1 != '\0') {
        ++str1; ++str2;
    }
    return (*str1 == *str2) ? 0 : ((*str1 < *str2) ? -1 : 1);
}
