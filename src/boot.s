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
.global _prcb
_prcb:
	.space	4
	.word	0x00000000
	.space	12
	.word	_int
	.word	_int_stack
	.space	4
	.word	0x0000027f
	.word	0x0000027f
	.word	0
	.word	0
	.space	32

.section .text
.global _start
.extern _kmain
.extern _data_load
.extern _data_start
.extern _data_end

init_irq:
	lda timer_irq, g0
	ldconst _int + (1 + 8) * 4, g1
	st g0, (g1)

	lda serial_irq, g0
	ldconst _int + (1 + 9) * 4, g1
	st g0, (g1)

	ldconst 0xff090800, g0
	ldconst 0xff000004, g1
	synmov g0, g1

	ret

copy_data:
	lda _data_load, g0
	lda _data_start, g1
	lda _data_end, g2
1:
	cmpobge g1, g2, 1f

	ld (g0), g3
	st g3, (g1)

	addo 4, g0, g0
	addo 4, g1, g1
	b 1b
1:
	ret

_start:
	ldconst 0x00180000, fp
	ldconst 0x00180040, sp
	mov 0, pfp

	call copy_data

	call init_irq
	call _kmain

	ldconst 0, g0
	shlo 16, g0, g0
	ldconst 15, g1
	shlo 16, g1, g1
	modpc g0, g1, g2

1:
	b 1b

.extern _uart_write
timer_irq:
	ldconst 'a', g0
	call _uart_write
	ret

serial_irq:
	ret

.section .int
_int:
	.space 1028
	.space 12
_int_stack:
