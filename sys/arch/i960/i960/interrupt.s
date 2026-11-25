.altmacro

.extern _intr_dispatch

.section .text
.global __intr_init
.global __intr_save
.global __intr_restore
.global __intr_set

.macro INTR_STUB n
__intr_\n:
	mov sp, r6
	ldconst 80, r5
	addo r5, sp, sp

	st g0, (r6)
	st g1, 4(r6)
	st g2, 8(r6)
	st g3, 12(r6)
	st g4, 16(r6)
	st g5, 20(r6)
	st g6, 24(r6)
	st g7, 28(r6)
	st g8, 32(r6)
	st g9, 36(r6)
	st g10, 40(r6)
	st g11, 44(r6)
	st g12, 48(r6)
	st g13, 52(r6)
	st g14, 56(r6)
	st pfp, 60(r6)

	ld -12(fp), r3
	st r3, 64(r6)

	ldconst 0xfffffff0, r3
	and r3, pfp, r3
	addo 8, r3, r3

	flushreg

	ld (r3), r3
	st r3, 68(r6)

	ld -16(fp), r3
	st r3, 72(r6)

	modtc 0, 0, r3
	st r3, 76(r6)

	ldconst \n, g0
	mov r6, g1

	call _intr_dispatch

	ldconst 80, r5
	subo r5, sp, r6
	ld (r6), g0
	ld 4(r6), g1
	mov r6, sp

	ret
.endm

.set vec, 8
.rept (256-8)
INTR_STUB %vec
.set vec, vec + 1
.endr

.macro INTR_REG n
	lda __intr_\n, r3
	lda _int + (\n + 1) * 4, r4
	st r3, (r4)
.endm

__intr_init:
.set vec, 8
.rept (256-8)
INTR_REG %vec
.set vec, vec + 1
.endr
	mov 0, r3
	ldconst 0x1f0000, r4
	modpc r4, r4, r3
	ret

__intr_save:
	modpc 0, 0, g0

	ldconst 0x1f0000, r3
	modpc r3, r3, r3
	ret

__intr_restore:
	ldconst 0x1f0000, r3
	modpc r3, r3, g0
	ret

__intr_set:
	modpc 0, 0, r5
	ldconst 0x1f0000, r6
	modpc r6, r6, r6

	mov g0, r3
	addo 1, r3, r3
	mulo 4, r3, r3
	lda _int, r4
	addo r4, r3, r3

	st g1, (r3)

	ldconst 0x1f0000, r6
	modpc r6, r6, r5

	ret

.section .int
.global _int
.global _int_stack
_int:
	.space 1028
	.space 12
_int_stack:
	.space 4096
