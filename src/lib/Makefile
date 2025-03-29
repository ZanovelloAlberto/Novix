BUILD_DIR?=build

ASM?=nasm
ASMFLAGS= -f elf

LIBGCC_PATH?=/home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
CC?=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g -I $(abspath $(LIBGCC_PATH))

LD?=/home/novice/cross/i686-elf/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib -L $(LIBGCC_PATH)

SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/lib/c/%.obj, $(SOURCES_C))

lib: $(OBJECTS_C)

$(BUILD_DIR)/lib/c/%.obj: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAG) -c $< -o $@
	@echo "--> Compiled: " $<