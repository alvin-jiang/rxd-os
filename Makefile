#########################
#  Makefile for RXD-OS  #
#########################

ENTRYPOINT	= 0x0000

# Programs, flags, etc.
# so "echo -e" will not output "-e"
SHELL	= /bin/bash

ASM		= nasm
# flags for boot
ASMBIN	= -I boot/
# flags for kernel
ASMOBJ	= -I boot/ -f elf32

CC		= gcc
CFLAGS	= -I include -I kernel/blk_drv -m32 -c -nostdlib -fno-builtin -fno-stack-protector -Werror -Wall
#-Wpadded -Wall
LD		= ld
LDFLAGS		= -m elf_i386 --oformat=binary -Map krnl.map -Ttext $(ENTRYPOINT)
#LDFLAGS	= -m elf_i386 -Map krnl.map -Ttext $(ENTRYPOINT)

szfiles		= .progsz .progsz.0

# This Program
#bootsrc != find ./boot/*.asm
bootsrc = ./boot/boot.asm ./boot/setup.asm
bootprog = $(bootsrc:.asm=.bin)

kernelsrc = ./boot/head.asm \
	$(wildcard ./kernel/*.asm) \
	$(wildcard ./kernel/*.c) \
	$(wildcard ./kernel/blk_drv/*.c) \
	$(wildcard ./mm/*.c) \
	$(wildcard ./fs/*.c)
kernelobjs = ./boot/head.o \
	$(patsubst %.asm, %.o, $(wildcard ./kernel/*.asm)) \
	$(patsubst %.c, %.o, $(wildcard ./kernel/*.c)) \
	$(patsubst %.c, %.o, $(wildcard ./kernel/blk_drv/*.c)) \
	$(patsubst %.c, %.o, $(wildcard ./mm/*.c)) \
	$(patsubst %.c, %.o, $(wildcard ./fs/*.c))

libsrc = $(wildcard ./lib/*.asm) $(wildcard ./lib/*.c)
libobjs = $(patsubst %.asm, %.o, $(wildcard ./lib/*.asm)) \
	$(patsubst %.c, %.o, $(wildcard ./lib/*.c))

kernelprog = kernel.bin
sysimg = rxdos.img

# All Phony Targets
.PHONY : rebuild source clean

# Default starting position
$(sysimg) : $(bootprog) $(kernelprog) $(szfiles)
	dd if=boot/boot.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=.progsz.0 of=$@ bs=1 seek=508 count=2 conv=notrunc
	dd if=boot/setup.bin of=$@ bs=512 conv=notrunc seek=1 count=`awk '{printf "%s", $$2}' .progsz`
	dd if=$(kernelprog) of=$@ bs=512 conv=notrunc seek=`awk '{printf "%s", $$2+1}' .progsz` count=`awk '{printf "%s", $$4}' .progsz`

rebuild : clean source

source : $(bootprog) $(kernelprog)

clean :
	rm -f $(bootprog) $(kernelprog) $(szfiles) $(kernelobjs) $(libobjs)

$(kernelprog) : $(kernelobjs) $(libobjs)
	$(LD) $(LDFLAGS) $^ -o $@

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.asm
	$(ASM) $(ASMOBJ) $< -o $@

%.bin : %.asm
	$(ASM) $(ASMBIN) $< -o $@

.progsz : boot/setup.bin $(kernelprog)
	du --apparent-size --block-size=512 boot/setup.bin | awk '{printf "%02x %d", $$1, $$1}' > $@
	@echo setup size = `awk '{printf "%s", $$2}' $@`
	du --apparent-size --block-size=512 $(kernelprog) | awk '{printf " %02x %d", $$1, $$1}' >> $@
	@echo kernel size = `awk '{printf "%s", $$4}' $@`

.progsz.0 : .progsz
	echo -e "\x`awk '{printf "%s", $$1}' $<`\x`awk '{printf "%s", $$3}' $<`" > $@

