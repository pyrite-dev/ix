OUTPUT_FORMAT(coff-Intel-little);

MEMORY {
	FLASH	(rx) : ORIGIN = 0x00000000, LENGTH = 1M
	RAM	(rwx) : ORIGIN = 0x00100000, LENGTH = 1M
}

SECTIONS {
	.boot 0 : {
		*(.boot);
	} > FLASH
	.sat 0x00001000 : {
		*(.sat);
	} > FLASH
	.sysproc 0x00002000 : {
		*(.sysproc);
	} > FLASH

	.text 0x00003000 : {
		*(.text);
		*(.rodata);
	} > FLASH

	.data : {
		. = . + 100;
		. = ALIGN(4);
		_data_start = .;
		*(.data);
		. = ALIGN(4);
		_data_end = .;
	} > RAM AT > FLASH
	_data_load = LOADADDR(.data) + 100;

	.prcb ORIGIN(RAM) - 80 : {
		*(.prcb);
	} > FLASH

	.bss (NOLOAD) : {
		. = ALIGN(4);
		_bss_start = .;
		*(.bss);
		*(COMMON);
		. = ALIGN(4);
		_kernel_stack = .;
		. += 4096;
	} > RAM

	.int (NOLOAD) : {
		. = ALIGN(4);
		*(.int);
		. = ALIGN(4);
		_bss_end = .;
	} > RAM
}

_checksum = -(_sat + _prcb + _start);
