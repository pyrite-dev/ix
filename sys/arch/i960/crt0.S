.section .text
.global _start

.extern _data_load
.extern _data_start
.extern _data_end
.extern _bss_start
.extern _bss_end

#ifdef __KERNEL__
.extern _kernel_stack
#endif

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

init_sections:
	call copy_data
	call clear_bss
	ret

_start:
#ifdef __KERNEL__
	mov 0, fp
	mov 0, pfp
	lda _kernel_stack, pfp
#endif

	call init_sections

#ifdef __KERNEL__
	call _kmain
1:
	b 1b
#else
	call _main
#endif
