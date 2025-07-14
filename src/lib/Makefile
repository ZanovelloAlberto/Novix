BUILD_DIR?=build

ASM?=nasm
ASMFLAGS= -f elf

CC?=zig cc -target i686-elf
CFLAG=-m32 -march=i386 -ffreestanding -nostdlib -std=c99 -g -fno-stack-protector

LD?=zig cc -target i686-elf
LDFLAG=-nostdlib

SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/lib/c/%.obj, $(SOURCES_C))

lib: $(OBJECTS_C)

$(BUILD_DIR)/lib/c/%.obj: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAG) -c $< -o $@
	@echo "--> Compiled: " $<
