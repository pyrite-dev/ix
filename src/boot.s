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
	.word	0x00000000
	.word	0x00000000
	.space	32

.section .text
.global _start
.extern _kmain
.extern _data_load
.extern _data_start
.extern _data_end
.extern _bss_start
.extern _bss_end

.extern _uart_write

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

clear_bss:
	lda _bss_start, g0
	lda _bss_end, g1
1:
	cmpobge g0, g1, 1f
	
	mov 0, g2
	st g2, (g0)

	addo 4, g0, g0
	b 1b
1:
	ret

_icr:
	.word 0xff090800

init_irq:
	lda timer_irq, g0
	ldconst _int + (1 + 8) * 4, g1
	st g0, (g1)

	lda serial_irq, g0
	ldconst _int + (1 + 9) * 4, g1
	st g0, (g1)

	lda _icr, g0
	ldconst 0xff000004, g1
	synmov g1, g0

	ret

_start:
	ldconst 0x00180000, fp
	ldconst 0x00180040, sp
	mov 0, pfp

	call copy_data
	call clear_bss
	call init_irq

	call _kmain

	shlo 16, 0, g0
	shlo 16, 31, g1
	modpc g0, g1, g2

1:
	lda _letter, g3
	ld (g3), g0
	call _uart_write
	b 1b

timer_irq:
	lda _letter, r4
	ld (r4), r3

	ldconst 'z', r5
	cmpobe r3, r5, 1f
	b 2f
1:
	ldconst '0', r3
	b 3f
2:
	addo 1, r3, r3
3:
	st r3, (r4)
	ret

serial_irq:
	ret

.section .data
_letter:
	.word '0'

.section .int
_int:
	.space 1028
	.space 12
_int_stack:
	.space 512
