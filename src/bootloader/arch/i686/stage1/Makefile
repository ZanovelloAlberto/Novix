BUILD_DIR?=build

ASM?=nasm
ASMFLAGS= -f elf

LIBGCC_PATH?=/home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
CC?=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g -I $(abspath $(LIBGCC_PATH))

LD?=/home/novice/cross/i686-elf/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib -L $(LIBGCC_PATH)

stage1: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: boot.asm
	@$(ASM) boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin
	@echo "--> Created:  bootloader.bin"