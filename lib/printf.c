/**
 *
 * @file: printf.c
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-03-26
 *
 */

#include "stdio.h"

#include "stdarg.h" /* va_list */
#include "stddef.h"
#include "assert.h"
#include "string.h" /* strlen, strcpy */

#define EOF (0)
#define INT_MIN (0x80000000)
#define stdout ((FILE *)1)

/* ctype.h */
#define ISCNTRL 1
#define ISBLANK 2
#define ISSPACE 4
#define ISUPPER 8
#define ISLOWER 16
#define ISALPHA 32
#define ISDIGIT 64
#define ISXDIGIT 128
#define ISALNUM 256
#define ISPUNCT 512
#define ISGRAPH 1024
#define ISPRINT 2048
static const short chr_class_table[256] = {
    0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x7,0x5,0x5,0x5,0x5,0x1,0x1,
    0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,
    0x806,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,
    0xdc0,0xdc0,0xdc0,0xdc0,0xdc0,0xdc0,0xdc0,0xdc0,0xdc0,0xdc0,0xe00,0xe00,0xe00,0xe00,0xe00,0xe00,
    0xe00,0xda8,0xda8,0xda8,0xda8,0xda8,0xda8,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,
    0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xd28,0xe00,0xe00,0xe00,0xe00,0xe00,
    0xe00,0xdb0,0xdb0,0xdb0,0xdb0,0xdb0,0xdb0,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,
    0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xd30,0xe00,0xe00,0xe00,0xe00,0x1,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
};

#define DEF_ISXXX(_name, _chr_class) \
static inline int _name (int c) \
{ \
    return (chr_class_table[(unsigned char)c] & _chr_class) ? 1 : 0; \
}

/* Is character alphanumeric? */
DEF_ISXXX(isalnum, ISALNUM)
/* Is character alphabetic? */
DEF_ISXXX(isalpha, ISALPHA)
/* Is character blank? [C++11] */
DEF_ISXXX(isblank, ISBLANK);
/* Is character control character? */
DEF_ISXXX(iscntrl, ISCNTRL);
/* Is character decimal digit? */
DEF_ISXXX(isdigit, ISDIGIT);
/* Does character has graphical representation? */
DEF_ISXXX(isgraph, ISGRAPH);
/* Is character lowercase letter? */
DEF_ISXXX(islower, ISLOWER);
/* Is character printable? */
DEF_ISXXX(isprint, ISPRINT);
/* Is character a punctuation character? */
DEF_ISXXX(ispunct, ISPUNCT);
/* Is character a white-space? */
DEF_ISXXX(isspace, ISSPACE);
/* Is character uppercase letter? */
DEF_ISXXX(isupper, ISUPPER);
/* Is character hexadecimal digit? */
DEF_ISXXX(isxdigit, ISXDIGIT);

static inline int tolower (int c)
{
    return isupper(c) ? (c - 'A' + 'a') : c;
}
static inline int toupper (int c)
{
    return islower(c) ? (c - 'a' + 'A') : c;
}

/* stdlib.h */
static int atoi (const char * str)
{
    if (str == NULL)
        return 0;

    const char *p = str;
    int negative = 0, value = 0;

    while (isspace(*p))
        ++p;
    if (*p == '+' || *p == '-') {
        negative = (*p == '-') ? 1 : 0;
        ++p;
    }

    if (!isdigit(*p))
        return 0;

    do {
        value *= 10;
        value += (*p - '0');
        ++p;
    } while (isdigit(*p));

    return (negative && value != INT_MIN) ? -value : value;
}


#define __check_not_null(p) assert(p != NULL)
/* check if addr is in [low, high) */
#define __bound_check(addr, low, high) \
    assert( (addr) >= (low) && (addr) < (high) )
#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

/* buffer size of vfprintf() */
#define BUFFER_SIZE 2048

/*
    Format: %[flags][width][.precision][length]specifier
*/
#define FMT_ESCAPE '%'
#define FMT_FLAGS 1
#define FMT_NUMBER 2 /* digital part of [width] and [.precision] */
#define FMT_PRECISION 3
#define FMT_LENGTH 4
#define FMT_SPECIFIER 5
static const char format_table[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,0,1,0,5,0,0,0,0,2,1,0,1,3,0,
    1,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,
    0,5,0,0,0,5,5,5,0,0,0,0,4,0,0,0,
    0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,
    0,5,0,5,5,5,5,5,4,5,4,0,4,0,5,5,
    5,0,0,5,4,5,0,0,5,0,4,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/* [flags] */
#define FMT_F_LEFT_JUST 1 /* '-': left justification */
#define FMT_F_SIGN_PLUS 2 /* '+': add '+' for positive number */
#define FMT_F_SIGN_SPCE 4 /* ' ': add ' ' for positive number */
/* '#':
    add preceeding 0 for specifiers "oxX"
    force show decimal point for specifiers "aAeEfFgG"
*/
#define FMT_F_MARK      8
#define FMT_F_ZERO_PAD  16 /* '0': pad with '0' instead of ' ' */

/*
    convert integer to string,
    value is considered signed when base is 10, unsigned otherwise.
    NOTE: this is NOT a standard function!
*/
static char * __itoa ( int value, char * str, int base )
{
    static const char * STR_INT_MIN = "-2147483648";
    unsigned int uvalue = 0;
    char *tmp = NULL, c;

    char *p = str;
    if (value < 0 && base == 10) {
        if (value == INT_MIN) {
            strcpy(str, STR_INT_MIN);
            return str;
        }
        uvalue = (unsigned int)(-value);
        *(p++) = '-';
    }
    else
        uvalue = (unsigned int)value;

    /* convert */
    if (uvalue == 0)
        *(p++) = '0';
    else {
        while (uvalue) {
            *p = uvalue % base;
            *p = (*p < 10) ? (*p + '0') : (*p - 10 + 'a');
            ++p;
            uvalue /= base;
        }
    }
    *(p--) = '\0';

    /* reverse string */
    tmp = str;
    if (*tmp == '-')
        ++tmp;
    while (p > tmp) {
        c = *p;
        *(p--) = *tmp;
        *(tmp++) = c;
    }

    return str;
}

/* shift [first, end) by 'shift' and pad with 'pad' */
#define shift_pad(first, end, shift, pad) do { \
    assert(first < end && shift != 0); \
    char * _t; \
    if (shift > 0) { \
        _t = end; \
        do { \
            --end; \
            *(end + shift) = *end; \
        } while (end != first); \
        end += shift; \
        do { \
            *(--end) = pad; \
        } while (end != first); \
        end = _t; \
    } \
    else if (shift < 0) { \
        _t = first; \
        do { \
            *(first + shift) = *first; \
            ++first; \
        } while (end != first); \
        first -= shift; \
        do { \
            *(first++) = pad; \
        } while (end != first); \
        first = _t; \
    } \
} while (0)

/* extended __itoa, knows how to deal with format */
static char * __itoaf ( int value, char * str, int base,
    const char flags, const int width, const int precision )
{
    /* make sure only one flag is active */
    assert((flags&(flags-1)) == 0);

    /* deal with prefix */
    char *p = str;
    if (base == 10 && value > 0) {
        if (flags & FMT_F_SIGN_PLUS)
            *(p++) = '+';
        else if (flags & FMT_F_SIGN_SPCE)
            *(p++) = ' ';
    }
    else if (base == 8 || base == 16) {
        if (flags & FMT_F_MARK) {
            *(p++) = '0';
            if (base == 16)
                *(p++) = 'x';
        }
    }

    int padcnt = 0;
    /* deal with number and precision */
    char *nstart = p;
    char *nend = nstart + strlen(__itoa(value, nstart, base));
    if (*nstart == '-')
        ++nstart;
    if ((nstart + precision) > nend) {
        padcnt = precision - (nend - nstart);
        shift_pad(nstart, nend, padcnt, '0');
    }
    nend += padcnt;
    p = nend;

    /* deal with width and alignment */
    if ((str + width) > p) {
        if (flags & FMT_F_LEFT_JUST) {
            while ((str + width) > p)
                *(p++) = ' ';
            *p = '\0';
        }
        else {
            padcnt = width - (p - str);
            if (precision < 0 && (flags & FMT_F_ZERO_PAD)) {
                shift_pad(nstart, p, padcnt, '0');
            }
            else {
                shift_pad(str, p, padcnt, ' ');
            }
            *(p + padcnt) = '\0';
        }
    }

    return str;
}

/* print argument in accord with specification */
#define __print_arg(out, args, specifier, flags, width, precision, length, start) \
    do { \
        switch (specifier) { \
        case 'c': \
            *(out++) = va_arg(args, int); \
            break; \
        case 's': \
            out += strlen(strcpy(out, va_arg(args, char *))); \
            break; \
        case 'd': \
        case 'i': \
            out += strlen(__itoaf(va_arg(args, int), out, 10, flags, width, precision)); \
            break; \
        case 'p': \
            *(out++) = 'b'; \
            /* continue below */ \
        case 'x': \
            out += strlen(__itoaf(va_arg(args, int), out, 16, flags, width, precision)); \
            break; \
        case 'X': \
            __itoaf(va_arg(args, int), out, 16, flags, width, precision); \
            while (*out != '\0') { \
                if (islower(*out)) \
                    *out = toupper(*out); \
                ++out; \
            } \
            break; \
        case 'o': \
            out += strlen(__itoaf(va_arg(args, int), out, 8, flags, width, precision)); \
            break; \
        case 'n': /* write number of chars written so far to argument */ \
            *va_arg(args, int *) = out - start; \
            break; \
        case '%': \
            ++out; \
            break; \
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A': \
            panic("Not implemented specifier."); \
        default: \
            panic("Invalid specifier."); \
        } \
    } while (0);

int vfprintf ( FILE * stream, const char * fmt, va_list args )
{
    __check_not_null(fmt);

    if (stream != stdout)
        panic("IO: vfprinf only support stdout for now.");

    char buffer[BUFFER_SIZE];

    int ret = 0;
    ret = vsprintf(buffer, fmt, args);
    assert(ret <= BUFFER_SIZE);
    if (ret > 0) {
        puts(buffer);
    }
    return ret;
}

int vsprintf (char * str, const char * fmt, va_list args )
{
    __check_not_null(str);
    __check_not_null(fmt);

    int escape = 0, bad = 0;

    char flags = 0;
    int width = 0, precision = -1/*, length = 0*/;
    int *width_or_precision = &width;
    
    const char *in = fmt;
    char *out = str;
    while (*in != '\0' && !bad) {
        *out = *(in++);
        if (!escape && *out == FMT_ESCAPE) {
            escape = 1;
        }
        else {
            if (escape) {
                switch (format_table[*((unsigned char *)out)]) {
                case FMT_FLAGS:
                    // NOTE: this is a little bit hacky, we need to
                    // let '0' go down for [width] and [.precision]
                    // if it's not flag '0'.
                    if (escape == 1) {
                        escape = 2;
                        switch(*out) {
                        case '-': flags |= FMT_F_LEFT_JUST; break;
                        case '+': flags |= FMT_F_SIGN_PLUS; break;
                        case ' ': flags |= FMT_F_SIGN_SPCE; break;
                        case '#': flags |= FMT_F_MARK; break;
                        case '0': flags |= FMT_F_ZERO_PAD; break;
                        default: break;
                        }
                        break;
                    }
                    else if ((escape == 2 || escape == 3) && *out == '0') {
                        /* '0' goes down here! jump to case FMT_NUMBER */
                    }
                    else { bad = 1; break; }
                case FMT_NUMBER: /* for [width] and [precision] */
                    escape = max(escape, 2);
                    if (escape == 2 || escape == 3) {
                        escape += 1;
                        if (*out == '*')
                            *width_or_precision = va_arg(args, int);
                        else {
                            *width_or_precision = atoi(in - 1);
                            while(isdigit(*in))
                                ++in;
                        }
                        break;
                    }
                    else { bad = 1; break; }
                case FMT_PRECISION:
                    escape = max(escape, 3);
                    if (escape == 3) {
                        width_or_precision = &precision;
                        break;
                    }
                    else { bad = 1; break; }
                case FMT_LENGTH:
                    escape = max(escape, 4);
                    /* ignore for now */
                    if (escape == 4) {
                        escape = 5;
                        while (format_table[*((unsigned char *)in)] == FMT_LENGTH)
                            ++in;
                        break;
                    }
                    else { bad = 1; break; }
                case FMT_SPECIFIER:
                    escape = max(escape, 5);
                    if (escape == 5) {
                        __print_arg(out, args, *out, flags, 
                            width, precision, 0/*length*/, str);
                        escape = 0;
                        flags = 0;
                        width = /*length = */0;
                        precision = -1;
                        width_or_precision = &width;
                        break;
                    }
                    else { bad = 1; break; }
                default: /* bad format */
                    bad = 1; break;
                }
            }
            else /* !escape && *out != FMT_ESCAPE */
                ++out;
        }
    }
    *(out) = '\0';

    /* NOTE: return length NOT includes '\0' */
    return bad ? -1 : (out - str);
}

int sprintf (char * str, const char * fmt, ...)
{
    __check_not_null(str);
    __check_not_null(fmt);

    int ret = 0;
    va_list args;
    va_start(args, fmt);
    ret = vsprintf(str, fmt, args);
    va_end(args);
    return ret;
}

int printf (const char * fmt, ...)
{
    __check_not_null(fmt);

    int ret = 0;
    va_list args;
    va_start(args, fmt);
    ret = vfprintf(stdout, fmt, args);
    va_end(args);
    return ret;
}
