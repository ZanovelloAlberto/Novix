BUILD_DIR?=build
LIB_DIR = ../../lib
ASM=nasm
ASMFLAGS= -f elf
CC?=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g -I $(abspath $(LIB_DIR))
LD?=/home/novice/cross/i686-elf/bin/i686-elf-ld
LIBGCC_PATH?=/home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
LDFLAG=-T linker.ld -nostdlib -L $(LIBGCC_PATH)

SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/lib/c/%.obj, $(SOURCES_C))

lib: $(OBJECTS_C)

$(BUILD_DIR)/lib/c/%.obj: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAG) -c $< -o $@
	echo "--> Compiled: " $<