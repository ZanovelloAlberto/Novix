ENTRY(_start)


KERNEL_ADDR_OFFSET = 0xC0000000;
KERNEL_VADDR_START = 0xC0100000;


SECTIONS {
    . = 2M;
	  KERNEL_PHYSADDR_START = .; 
    .text : ALIGN(4K) {
        KEEP(*(.multiboot))
        *(.text)
    }

	.rodata ALIGN(4K) : {
		*(.rodata)
	}

	.data ALIGN(4K) : {
		*(.data)
	}

	.bss ALIGN(4K) : {
		*(COMMON)
		*(.bss)
	}



	KERNEL_VADDR_END = .;
	KERNEL_PHYSADDR_END = . - KERNEL_ADDR_OFFSET;
}
