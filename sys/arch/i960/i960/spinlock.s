.section .text
.global __spinlock_trylock
.global __spinlock_unlock

__spinlock_trylock:
	mov 1, r3
	atmod g0, 1, r3

	and 1, r3, r3
	cmpo 0, r3
	bne 1f

	mov 1, g0
	ret

1:
	mov 0, g0
	ret

__spinlock_unlock:
	mov 0, r3
	atmod g0, 1, r3
	ret
