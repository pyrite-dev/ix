.altmacro

.extern _intr_dispatch

.section .text
.global __intr_init
.global __intr_save
.global __intr_restore

.macro INTR_STUB n
intr_\n:
	mov g0, r3
	ldconst \n, g0
	call _intr_dispatch
	mov r3, g0
	ret
.endm

.set vec, 8
.rept (256-8)
INTR_STUB %vec
.set vec, vec + 1
.endr

.macro INTR_REG n
	lda intr_\n, r3
	lda _int + (\n + 1) * 4, r4
	st r3, (r4)
.endm

__intr_init:
.set vec, 8
.rept (256-8)
INTR_REG %vec
.set vec, vec + 1
.endr
	shlo 16, 0, r3
	shlo 16, 31, r4
	modpc r3, r4, r5
	ret

__intr_save:
	modpc g0, 0, g0

	shlo 16, 31, r3
	modpc r3, r3, r3

	ret

__intr_restore:
	shlo 16, 31, r3
	modpc g0, r3, r3
	ret

.section .int
.global _int
.global _int_stack
_int:
	.space 1028
	.space 12
_int_stack:
	.space 4096
