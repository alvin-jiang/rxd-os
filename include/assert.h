#ifndef __ASSERT_H__
#define __ASSERT_H__

#include "stdio.h"

// __FILE__, __BASE_FILE__, __LINE__
#define assert(exp) do { \
    if (exp) ; \
    else { \
        printf("!!! assert failed: " #exp "\n!!! at %s : %d\n", __FILE__, __LINE__); \
        while(1); } \
    } while(0)

#define panic(msg) do { \
    printf("!!! PANIC -> " msg "\n"); \
    assert(0); \
    } while(0)

#endif