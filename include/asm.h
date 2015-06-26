#ifndef __ASM_H__
#define __ASM_H__

#define cli() __asm__ ("cli"::)
#define sti() __asm__ ("sti"::)

#define klock() __asm__ ("pushf\n\tcli"::)
#define kunlock() __asm__ ("popf"::)

/* inb/inw/inl */
static inline u8 inb (u16 port)
{
    unsigned char _v;
    __asm__ volatile ("inb %%dx,%%al":"=a" (_v):"d" (port));
    return _v;
}

/* inb_p/inw_p/inl_p */
static inline u8 inb_p (u16 port)
{
    unsigned char _v;
    __asm__ volatile ("inb %%dx,%%al\n\t" \
        "jmp 1f\n" \
        "1:\tjmp 1f\n" \
        "1:":"=a" (_v):"d" (port));
    return _v;
}

/* outb/outw/outl */
static inline void outb (u16 port, u8 value)
{
    __asm__ ("outb %%al,%%dx"::"a" (value),"d" (port));
}

/* outb_p/outw_p/outl_p */
static inline void outb_p (u16 port, u8 value)
{
    __asm__ ("outb %%al,%%dx\n\t" \
        "jmp 1f\n" \
        "1:\tjmp 1f\n" \
        "1:"::"a" (value),"d" (port));
}

/* insb/insw/insl */
static inline void port_read(u16 port, void * buf, int nr) {
    nr = (nr + 1) >> 1;
    __asm__("cld;rep;insw"
        :"=d"(port),"=D"(buf),"=c"(nr)
        :"0"(port),"1"(buf),"2"(nr)
        :"memory");
}
/* outsb/outsw/outsl */
static inline void port_write(u16 port, void * buf, int nr) {
    nr = (nr + 1) >> 1;
    __asm__("cld;rep;outsw"
        :"=d"(port),"=S"(buf),"=c"(nr)
        :"0"(port),"1"(buf),"2"(nr)
        :"memory");
}

#endif