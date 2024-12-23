BUILD_DIR?=build/
SRC_DIR=src/
ASM=nasm
ASMFLAGS= -f elf
CC= /home/novice/opt/cross/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g
LD=/home/novice/opt/cross/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib

stage1: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: boot.asm
	$(ASM) boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin