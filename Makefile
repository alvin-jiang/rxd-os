#########################
# Makefile for Orange'S #
#########################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
ENTRYPOINT	= 0x1000

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
SHELL	= /bin/bash # so "echo -e" will not output "-e"

ASM		= nasm
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -I include/sys/ -f elf

DASM	= objdump
DASMFLAGS	= -D

CC		= gcc
CFLAGS	= -I include/ -I include/sys/ -m32 -c -fno-builtin -fno-stack-protector
LD		= ld
LDFLAGS		= -m elf_i386 -Ttext $(ENTRYPOINT) -Map krnl.map

szfiles		= .setupsz .setupsz.0

# This Program
ASMSRC = $(wildcard ./boot/*.asm)
ASMOUT = $(ASMSRC:.asm=.bin)

# All Phony Targets
.PHONY : all image clean

# Default starting position
all : $(ASMOUT) $(szfiles)

image : $(ASMOUT) $(szfiles)
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	dd if=.setupsz.0 of=a.img bs=1 seek=508 count=1 conv=notrunc
	dd if=boot/setup.bin of=a.img bs=512 seek=1 conv=notrunc count=`cat .setupsz`

clean :
	rm -v $(ASMOUT) $(szfiles)

%.bin : %.asm
	$(ASM) $(ASMBFLAGS) $< -o $@

.setupsz : boot/setup.bin
	du --apparent-size --block-size=512 boot/setup.bin | awk '{printf "%02x", $$1}' > $@

.setupsz.0 : .setupsz
	echo -e "\x`cat $<`" > $@

