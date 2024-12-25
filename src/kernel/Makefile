BUILD_DIR?=build/
ASM=nasm
ASMFLAGS= -f elf
CC=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g
LD=/home/novice/cross/i686-elf/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib -L /home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0/


SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard *.asm)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))

kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(LD) $(LDFLAG) -o $@ $^ -lgcc

$(BUILD_DIR)/kernel/c/%.obj: %.c
	$(CC) $(CFLAG) -c $< -o $@ 

$(BUILD_DIR)/kernel/asm/%.obj: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<
