ENTRY(_start)

/* Changes to KERNEL_ADDR_OFFSET must also be made to KERNEL_PAGE_NUMBER in boot.zig */
KERNEL_ADDR_OFFSET = 0xC0000000;
KERNEL_VADDR_START = 0xC0100000;

SECTIONS {
	. = 1M;
	KERNEL_PHYSADDR_START = .;

	.rodata.boot : {
		*(.rodata.boot)
	}

	.text.boot : {
		*(.text.boot)
	}

	. += KERNEL_ADDR_OFFSET;

	.text ALIGN(4K) : AT (ADDR(.text) - KERNEL_ADDR_OFFSET) {
		*(.text)
	}

	.rodata ALIGN(4K) : AT (ADDR(.rodata) - KERNEL_ADDR_OFFSET) {
		*(.rodata)
	}

	.data ALIGN(4K) : AT (ADDR(.data) - KERNEL_ADDR_OFFSET) {
		*(.data)
	}

	.bss ALIGN(4K) : AT (ADDR(.bss) - KERNEL_ADDR_OFFSET) {
		*(COMMON)
		*(.bss)
	}


	KERNEL_VADDR_END = .;
	KERNEL_PHYSADDR_END = . - KERNEL_ADDR_OFFSET;

}
