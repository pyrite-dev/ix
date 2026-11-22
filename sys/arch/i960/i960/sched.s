.extern _sched_started
.extern _sched_tick
.extern _sched_preempt
.extern _ticks

.section .text
.global __sched_intr

__sched_intr:
	ldconst 0x001f0000, r4
	modpc r4, r4, r4

	ld _ticks, r3
	addo 1, r3, r3
	st r3, _ticks

	ld _sched_started, r3
	cmpobe 0, r3, 3f

	mov sp, r3
	ldconst 72, r4
	addo r4, r3, sp
	st g0, (r3)
	st g1, 4(r3)
	st g2, 8(r3)
	st g3, 12(r3)
	st g4, 16(r3)
	st g5, 20(r3)
	st g6, 24(r3)
	st g7, 28(r3)
	st g8, 32(r3)
	st g9, 36(r3)
	st g10, 40(r3)
	st g11, 44(r3)
	st g12, 48(r3)
	st g13, 52(r3)
	st g14, 56(r3)

	ldl -16(fp), r4
	st r4, 60(r3)
	st r5, 64(r3)
	st pfp, 68(r3)

	call _sched_tick

	mov r3, g0
	call _sched_preempt

	ld 68(r3), r4
	and 7, r4, r4
	cmpobe 7, r4, 2f

1:	# context switch
	subo 1, 0, r5
	ld 64(r3), r4
	modac r5, r4, r4

	flushreg

	ldq (r3), g0
	ldq 16(r3), g4
	ldq 32(r3), g8
	ldt 48(r3), g12

	ld 68(r3), r4
	mov r4, pfp

	ldconst 0x001f2002, r5
	ld 60(r3), r4
	modpc r5, r5, r4

	ret

2:	# no context switch
	ldq (r3), g0
	ldq 16(r3), g4
	ldq 32(r3), g8
	ldt 48(r3), g12
3:	# scheduler not ready
	ret
