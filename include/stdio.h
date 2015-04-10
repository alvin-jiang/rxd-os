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

// klib.asm
void printk (const char *s);

// printf.c
int printf (const char *fmt, ...);
int sprintf (char *s, const char *fmt, ...);
// int puts (const char *s);
#define puts(str) printk(str)

#endif

