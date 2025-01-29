BUILD_DIR?=build/
ASM=nasm
ASMFLAGS= -f elf
CC?=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g
LD?=/home/novice/cross/i686-elf/bin/i686-elf-ld
LIBGCC_PATH?=/home/novice/opt/cross/i686-elf/lib/gcc/i686-elf/14.2.0/
LDFLAG=-T linker.ld -nostdlib -L $(LIBGCC_PATH)


SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard *.asm)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/bootloader/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/bootloader/asm/%.obj, $(SOURCES_ASM))

stage2: $(BUILD_DIR)/boot0.bin

$(BUILD_DIR)/boot0.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	@mkdir -p $(@D)
	@$(LD) $(LDFLAG) -o $@ $^ -lgcc
	@echo "--> Created:  boot0.bin"

$(BUILD_DIR)/bootloader/c/%.obj: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAG) -c $< -o $@
	@echo "--> Compiled: " $<

$(BUILD_DIR)/bootloader/asm/%.obj: %.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<
