BUILD_DIR=build
SRC_DIR=src
FAT=mkfs.fat
ASM=nasm
ASMFLAGS= -f elf
CC=/home/novice/opt/cross/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g
LD=/home/novice/opt/cross/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib
LIBGCC_PATH= /home/novice/opt/cross/i686-elf/lib/gcc/i686-elf/14.2.0/


#
# Floppy image
#

floppy_image: $(BUILD_DIR)/main.img

$(BUILD_DIR)/main.img: bootloader
	dd if=/dev/zero of=$(BUILD_DIR)/main.img bs=512 count=2880
	$(FAT) -F 12 -n "NOVIX" $(BUILD_DIR)/main.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main.img $(BUILD_DIR)/boot0.bin "::boot0.bin"
	mcopy -i $(BUILD_DIR)/main.img $(BUILD_DIR)/kernel.bin "::kernel.bin"


#
# Bootloader
#

bootloader: stage1 stage2 kernel

stage1:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1 BUILD_DIR=$(abspath $(BUILD_DIR))

stage2:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2 BUILD_DIR=$(abspath $(BUILD_DIR)) LD=$(LD) CC=$(CC) LIBGCC_PATH=$(LIBGCC_PATH)

kernel:
	$(MAKE) -C $(SRC_DIR)/kernel/ BUILD_DIR=$(abspath $(BUILD_DIR)) LD=$(LD) CC=$(CC) LIBGCC_PATH=$(LIBGCC_PATH)

#
# For Novice on windows Cygwin
#

cygwin: CC =  /home/Novice/opt/cross/bin/i686-elf-gcc.exe
cygwin: LD = /home/Novice/opt/cross/bin/i686-elf-ld.exe
cygwin: FAT=/usr/local/sbin/mkfs.fat.exe
cygwin: LIBGCC_PATH=/home/novice/opt/cross/lib/gcc/i686-elf/14.2.0
cygwin: floppy_image

clean:
	rm -rf build/*