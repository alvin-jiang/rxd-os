/**
 *
 * @file: syscall.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-05-20
 *
 */


#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define __NR_fork 0
#define __NR_open 1
#define __NR_close 2
#define __NR_read 3
#define __NR_write 4

/* marcos for syscall */
#define _syscall0(type,name) \
type name(void) \
{ \
long __res; \
__asm__ volatile ("int $0x90" \
    : "=a" (__res) \
    : "0" (__NR_##name)); \
    return (type) __res; \
}
/*if (__res >= 0) \
    return (type) __res; \
errno = -__res; \
return -1; \*/

#define _syscall1(type,name,atype,a) \
type name(atype a) \
{ \
long __res; \
__asm__ volatile ("int $0x90" \
    : "=a" (__res) \
    : "0" (__NR_##name),"b" ((long)(a))); \
    return (type) __res; \
}

#define _syscall2(type,name,atype,a,btype,b) \
type name(atype a,btype b) \
{ \
long __res; \
__asm__ volatile ("int $0x90" \
    : "=a" (__res) \
    : "0" (__NR_##name),"b" ((long)(a)),"c" ((long)(b))); \
    return (type) __res; \
}

#define _syscall3(type,name,atype,a,btype,b,ctype,c) \
type name(atype a,btype b,ctype c) \
{ \
long __res; \
__asm__ volatile ("int $0x90" \
    : "=a" (__res) \
    : "0" (__NR_##name),"b" ((long)(a)),"c" ((long)(b)),"d" ((long)(c))); \
    return (type) __res; \
}

#endif

