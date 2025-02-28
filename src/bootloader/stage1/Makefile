BUILD_DIR?=build
LIB_DIR = ../../lib
ASM=nasm
ASMFLAGS= -f elf
CC?=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g -I $(abspath $(LIB_DIR))
LD?=/home/novice/cross/i686-elf/bin/i686-elf-ld
LIBGCC_PATH?=/home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
LDFLAG=-T linker.ld -nostdlib -L $(LIBGCC_PATH)

stage1: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: boot.asm
	@$(ASM) boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin
	@echo "--> Created:  bootloader.bin"