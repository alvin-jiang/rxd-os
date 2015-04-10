/**
 *
 * @file: string.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-26
 *
 */
#ifndef __STRING_H__
#define __STRING_H__

#include "type.h"

void * memcpy (void * dst, const void * src, size_t num);
void * memmove (void * dst, const void * src, size_t num);
void * memset (void * ptr, int value, size_t num);
char * strcpy (char * dst, const char * src);
size_t strlen (const char * str);

#endif

