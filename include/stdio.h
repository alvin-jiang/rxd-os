/**
 *
 * @file: stdio.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-26
 *
 */
#ifndef __STDIO_H__
#define __STDIO_H__

// #define EOF (-1)
#include "stdarg.h"

// printk.asm
void printk (const char *s);

// printf.c
int printf (const char *fmt, ...);
int sprintf (char *s, const char *fmt, ...);
int vsprintf (char * str, const char * fmt, va_list args );
// int puts (const char *s);
#define puts(str) printk(str)

#endif

