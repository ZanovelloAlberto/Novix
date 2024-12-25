BUILD_DIR=build
SRC_DIR=src
ASM=nasm
ASMFLAGS= -f elf
CC=/home/novice/opt/cross/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g
LD=/home/novice/opt/cross/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib


#
# Floppy image
#

floppy_image: $(BUILD_DIR)/main.img

$(BUILD_DIR)/main.img: bootloader
	dd if=/dev/zero of=$(BUILD_DIR)/main.img bs=512 count=2880
	mkfs.fat -F 12 -n "NOVIX" $(BUILD_DIR)/main.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main.img $(BUILD_DIR)/boot0.bin "::boot0.bin"
	mcopy -i $(BUILD_DIR)/main.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	mmd -i $(BUILD_DIR)/main.img "::mydir"
	mcopy -i $(BUILD_DIR)/main.img file.txt "::mydir/file.txt"


#
# Bootloader
#

bootloader: stage1 stage2 kernel

stage1:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1 BUILD_DIR=$(abspath $(BUILD_DIR))

stage2:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2 BUILD_DIR=$(abspath $(BUILD_DIR))

kernel:
	$(MAKE) -C $(SRC_DIR)/kernel/ BUILD_DIR=$(abspath $(BUILD_DIR))

clean:
	rm -rf build/*
	mkdir build/bootloader
	mkdir build/bootloader/asm
	mkdir build/bootloader/c
	mkdir build/kernel
	mkdir build/kernel/c
	mkdir build/kernel/asm