.altmacro

.extern _syscall_dispatch
.extern _thread_current

.section .text
.global __syscall_entry
.global __syscall_invoke

__syscall_entry:
	mov sp, r3
	ldconst 80, r4
	addo r4, sp, sp

	stq g0, 0(r3)
	stq g4, 16(r3)
	stq g8, 32(r3)
	stt g12, 48(r3)

	mov pfp, r4
	st r4, 60(r3)
	modac 0, 0, r4
	st r4, 64(r3)
	mov fp, r4
	andnot 7, r4, r4
	ld 8(r4), r4
	st r4, 68(r3)
	modpc 0, 0, r4
	st r4, 72(r3)
	modtc 0, 0, r4
	st r4, 76(r3)

	mov r3, r4
	call _thread_current
	cmpobe 0, g0, 2f
	ld 24(g0), r5
	ld 28(g0), r6
	subo r5, r3, r7
	cmpobge r7, r6, 1f
	b 2f
1:
	mov r3, r8
	mov r5, r9
	ldconst 20, r10
3:
	ld (r8), r11
	st r11, (r9)
	addo 4, r8, r8
	addo 4, r9, r9
	subo 1, r10, r10
	cmpobne 0, r10, 3b
	mov r5, r4
	ldconst 80, r6
	addo r6, r5, sp
2:
	mov r4, g0
	call _syscall_dispatch
	ld (r4), g0
	mov r3, sp
	ret

__syscall_invoke:
	calls 0
	ret

.section .bss
.global __syscall_stack
.align 4
__syscall_stack:
	.space 4096
