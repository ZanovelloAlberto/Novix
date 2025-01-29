BUILD_DIR?=build/
ASM=nasm
ASMFLAGS= -f elf
CC?=/home/novice/cross/i686-elf/bin/i686-elf-gcc
CFLAG=-ffreestanding -nostdlib -std=c99 -g -I include
LD?=/home/novice/cross/i686-elf/bin/i686-elf-ld
LIBGCC_PATH?=/home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
LDFLAG=-T linker.ld -nostdlib -L $(LIBGCC_PATH)


HEADERS_C = $(wildcard *.h) \
			$(wildcard */*.h) \
			$(wildcard */*/*.h) \
			$(wildcard */*/*/*.h)

SOURCES_C =	$(wildcard *.c) \
			$(wildcard */*.c) \
			$(wildcard */*/*.c) \
			$(wildcard */*/*/*.c) \
			arch/i686/isrs_gen.c

HEADERS_ASM = $(wildcard *.inc) \
			  $(wildcard */*.inc) \
			  $(wildcard */*/*.inc) \
			  $(wildcard */*/*/*.inc) \
			  arch/i686/isrs_gen.inc

SOURCES_ASM =  $(wildcard *.asm) \
			  $(wildcard */*.asm) \
			  $(wildcard */*/*.asm) \
			  $(wildcard */*/*/*.asm) \

OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))

OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))

kernel: isr_generator $(BUILD_DIR)/kernel.bin

isr_generator:
	@sed -i 's/\r//' isr_generator.sh
	@./isr_generator.sh arch/i686/isrs_gen.c arch/i686/isrs_gen.inc
	@echo "--> Generating: arch/i686/isrs_gen.c arch/i686/isrs_gen.inc"

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	@$(LD) $(LDFLAG) -o $@ $^ -lgcc
	@echo "--> Created:  kernel.bin"

$(BUILD_DIR)/kernel/c/%.obj: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	@$(CC) $(CFLAG) -c $< -o $@ 
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/%.obj: %.asm $(HEADERS_ASM)
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<
