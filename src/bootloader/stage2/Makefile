BUILD_DIR?=build
LIB_DIR?=lib

ASM?=nasm
ASMFLAGS= -f elf

LIBGCC_PATH?=/home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
CC?=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g -I $(LIB_DIR)

LD?=/home/novice/cross/i686-elf/bin/i686-elf-ld
LDFLAG=-T linker.ld -nostdlib -L $(LIBGCC_PATH)


SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard *.asm)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/bootloader/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/bootloader/asm/%.obj, $(SOURCES_ASM))

LIB_OBJECT =	$(wildcard $(BUILD_DIR)/lib/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/*/*/*/*.obj) \

stage2: $(BUILD_DIR)/boot0.bin

$(BUILD_DIR)/boot0.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	@mkdir -p $(@D)
	@$(LD) $(LDFLAG) -o $@ $^ $(LIB_OBJECT) -lgcc -Map $(BUILD_DIR)/boot0.map
	@echo "--> Created:  boot0.bin"

$(BUILD_DIR)/bootloader/c/%.obj: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAG) -c $< -o $@
	@echo "--> Compiled: " $<

$(BUILD_DIR)/bootloader/asm/%.obj: %.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<
