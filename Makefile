
CC = ~/opt/cross/bin/i686-elf-gcc
ASM = nasm
CFLAG = -ffreestanding -Wall -Wextra -g -O2
SRC_DIR = src
BUILD_DIR = build


#
# Floppy image
#

floppy_image: $(BUILD_DIR)/main.img

$(BUILD_DIR)/main.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main.img bs=512 count=2880
	mkfs.fat -F 12 -n "VICEOS" $(BUILD_DIR)/main.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main.img $(BUILD_DIR)/kernel "::kernel.bin"

#
# Bootloader
#

bootloader: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: $(SRC_DIR)/bootloader/boot.s
	$(ASM) $(SRC_DIR)/bootloader/boot.s -f bin -o $(BUILD_DIR)/bootloader.bin

#
# Kernel
#

kernel: $(BUILD_DIR)/asm/main.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/vga.o $(BUILD_DIR)/asm/util.o
	~/opt/cross/bin/i686-elf-ld -T $(SRC_DIR)/kernel/linker.ld -o $(BUILD_DIR)/kernel $^

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel/kernel.c
	$(CC) $(CFLAG) -c $(SRC_DIR)/kernel/kernel.c -o $(BUILD_DIR)/kernel.o

$(BUILD_DIR)/vga.o: $(SRC_DIR)/kernel/vga.c
	$(CC) $(CFLAG) -c $(SRC_DIR)/kernel/vga.c -o $(BUILD_DIR)/vga.o

$(BUILD_DIR)/asm/main.o: $(SRC_DIR)/kernel/main.s
	$(ASM) $(SRC_DIR)/kernel/main.s -f elf -o $(BUILD_DIR)/asm/main.o

$(BUILD_DIR)/asm/util.o: $(SRC_DIR)/kernel/util.s
	$(ASM) $(SRC_DIR)/kernel/util.s -f elf -o $(BUILD_DIR)/asm/util.o



clean:
	rm -rf $(BUILD_DIR)/*
	mkdir $(BUILD_DIR)/asm