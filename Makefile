BUILD_DIR=build
SRC_DIR=src
FAT=mkfs.fat
ASM=nasm
CC=zig cc
LD=i686-elf-ld
# LIBGCC_PATH= /home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
LIB_DIR=$(abspath $(SRC_DIR)/lib/)

export FAT
export ASM
export CC
export LD
export LIBGCC_PATH
export LIB_DIR

#
# Floppy image
#

floppy_image: $(BUILD_DIR)/main.img
	echo hi

$(BUILD_DIR)/main.img: lib bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main.img bs=512 count=2880
	$(FAT) -F 12 -n "NOVIX" $(BUILD_DIR)/main.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main.img $(BUILD_DIR)/boot0.bin "::boot0.bin"
	mcopy -i $(BUILD_DIR)/main.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	mmd -i $(BUILD_DIR)/main.img ::mydir
	echo "Test message inside 'mydir' !" > test_msg.txt
	mcopy -i $(BUILD_DIR)/main.img test_msg.txt "::mydir/test_msg.txt"
	rm test_msg.txt
	$(ASM) $(SRC_DIR)/user/userprog.asm -f bin -o $(BUILD_DIR)/userprog.bin
	mcopy -i $(BUILD_DIR)/main.img $(BUILD_DIR)/userprog.bin "::userprog.bin"


#
# lib
#

lib:
	$(MAKE) -C $(LIB_DIR) BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Bootloader
#

bootloader: stage1 stage2

stage1:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1/ BUILD_DIR=$(abspath $(BUILD_DIR))

stage2:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2/ BUILD_DIR=$(abspath $(BUILD_DIR))

kernel:
	$(MAKE) -C $(SRC_DIR)/kernel/ BUILD_DIR=$(abspath $(BUILD_DIR))

clean:
	rm -rf build/*
