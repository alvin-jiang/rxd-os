#########################
# Makefile for Orange'S #
#########################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
ENTRYPOINT	= 0x0000

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
# so "echo -e" will not output "-e"
SHELL	= /bin/bash

ASM		= nasm
# flags for boot
ASMBIN	= -I boot/include/
# flags for kernel
ASMOBJ	= -I boot/include/ -f elf32

DASM	= objdump
DASMFLAGS	= -D

CC		= gcc
CFLAGS	= -I include/ -I include/sys/ -m32 -c -fno-builtin -fno-stack-protector
LD		= ld
LDFLAGS		= -m elf_i386 -Map krnl.map -Ttext $(ENTRYPOINT)

szfiles		= .progsz .progsz.0

# This Program
bootsrc = ./boot/boot.asm ./boot/setup.asm
bootprog = $(bootsrc:.asm=.bin)
kernelsrc = ./boot/head.asm
kernelobjs = ./boot/head.o
kernelprog = kernel.bin
sysimg = rxdos.img

# All Phony Targets
.PHONY : all clean

# Default starting position
$(sysimg) : $(bootprog) $(kernelprog) $(szfiles)
	dd if=boot/boot.bin of=$(sysimg) bs=512 count=1 conv=notrunc
	dd if=.progsz.0 of=$(sysimg) bs=1 seek=508 count=2 conv=notrunc
	dd if=boot/setup.bin of=$(sysimg) bs=512 conv=notrunc seek=1 count=`awk '{printf "%s", $$2}' .progsz`
	dd if=$(kernelprog) of=$(sysimg) bs=512 conv=notrunc seek=`awk '{printf "%s", $$2+1}' .progsz` count=`awk '{printf "%s", $$4}' .progsz`

all : $(bootprog) $(kernelprog)

clean :
	rm -f $(bootprog) $(kernelprog) $(szfiles) $(kernelobjs)

$(kernelprog) : $(kernelobjs)
	$(LD) $(LDFLAGS) $^ -o $@

./boot/head.o : ./boot/head.asm
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

