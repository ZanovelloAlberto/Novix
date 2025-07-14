BUILD_DIR?=build
LIB_DIR?=../lib

ASM?=nasm
ASMFLAGS= -f elf

CC?=zig cc -target i686-elf
CFLAG=-m32 -march=i386 -ffreestanding -nostdlib -std=c99 -g -I include -I $(abspath $(LIB_DIR)) -fno-stack-protector

LD?=zig cc -target i686-elf
LDFLAG=-T linker.ld -nostdlib -m elf_i386 -lc


HEADERS_C = $(wildcard *.h) \
			$(wildcard */*.h) \
			$(wildcard */*/*.h) \
			$(wildcard */*/*/*.h)

SOURCES_C =	$(wildcard *.c) \
			$(wildcard */*.c) \
			$(wildcard */*/*.c) \
			$(wildcard */*/*/*.c) \
			hal/isrs_gen.c

HEADERS_ASM = $(wildcard *.inc) \
			  $(wildcard */*.inc) \
			  $(wildcard */*/*.inc) \
			  $(wildcard */*/*/*.inc) \
			  hal/isrs_gen.inc

SOURCES_ASM =  $(wildcard *.asm) \
			  $(wildcard */*.asm) \
			  $(wildcard */*/*.asm) \
			  $(wildcard */*/*/*.asm) \

OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))

OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))

LIB_OBJECT =	$(wildcard $(BUILD_DIR)/lib/c/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/c/*/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/c/*/*/*.obj) \
			$(wildcard $(BUILD_DIR)/lib/c/*/*/*/*.obj) \

kernel: isr_generator $(BUILD_DIR)/kernel.bin

isr_generator: hal/isrs_gen.c hal/isrs_gen.inc
	sed -i 's/\r//' isr_generator.sh
	./isr_generator.sh hal/isrs_gen.c hal/isrs_gen.inc
	echo "--> Generating: hal/isrs_gen.c hal/isrs_gen.inc"

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	@echo "--> tring creating stuff"
	$(LD) $(LDFLAG)  $^ $(LIB_OBJECT)  -Map $(BUILD_DIR)/kernel.map -o $@ 
	@echo "--> Created:  kernel.bin"

$(BUILD_DIR)/kernel/c/%.obj: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	@$(CC) $(CFLAG) -c $< -o $@ 
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/%.obj: %.asm $(HEADERS_ASM)
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<
