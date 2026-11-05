.extern _start

.section .boot
	.word	_sat
	.word	_prcb
	.word	0x00000000
	.word	_start
	.word	-1
	.word	0x00000000
	.word	0x00000000
	.word	_checksum

.section .sat
.global  _sat
_sat:
	.space	136
	.word	_sat
	.word	0x00fc00fb
	.space	8
	.word	_sysproc
	.word	0x304400fb

.section .sysproc
.global	 _sysproc
_sysproc:
	.space	48

.section .prcb
.global  _prcb
_prcb:
	.space	4
	.word	0x00000000
	.space	12
	.word	_int
	.word	_int_stack
	.space	4
	.word	0x0000027f
	.word	0x0000027f
	.word	0x00000000
	.word	0x00000000
	.space	32

.section .int
_int:
	.space 1028
	.space 12
_int_stack:
	.space 4096
