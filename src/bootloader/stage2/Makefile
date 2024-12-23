BUILD_DIR?=build/
ASM=nasm
ASMFLAGS= -f elf
CC=/home/novice/opt/cross/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g
LD=/home/novice/opt/cross/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib


SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard *.asm)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/bootloader/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/bootloader/asm/%.obj, $(SOURCES_ASM))

stage2: $(BUILD_DIR)/boot0.bin

$(BUILD_DIR)/boot0.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(LD) $(LDFLAG) -o $@ $^ -Map=$(BUILD_DIR)/stage2.map

$(BUILD_DIR)/bootloader/c/%.obj: %.c
	$(CC) $(CFLAG)	-c $< -o $@ 

$(BUILD_DIR)/bootloader/asm/%.obj: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<
