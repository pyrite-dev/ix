#include <sys/spinlock.h>
#include <sys/interrupt.h>

#define BARRIER() __asm__ __volatile__("" ::: "memory");

void spinlock_init(spinlock_t* lock) {
	lock->lock = 0;
}

void spinlock_lock(spinlock_t* lock) {
	/* TODO: Disable preemption */

	while(!_spinlock_trylock(&lock->lock)) {
		while(lock->lock != 0) {
			BARRIER();
		}
	}

	BARRIER();
}

void spinlock_unlock(spinlock_t* lock) {
	BARRIER();

	_spinlock_unlock(&lock->lock);

	BARRIER();

	/* TODO: Enable preemption */
}

int spinlock_trylock(spinlock_t* lock) {
	/* TODO: Disable preemption */

	if(!_spinlock_trylock(&lock->lock)) {
		/* TODO: Enable preemption */
		return 0;
	}

	BARRIER();

	return 1;
}

intr_flags_t spinlock_lock_irqsave(spinlock_t* lock) {
	intr_flags_t flags = _intr_save();

	spinlock_lock(lock);

	return flags;
}

void spinlock_unlock_irqrestore(spinlock_t* lock, intr_flags_t flags) {
	spinlock_unlock(lock);

	_intr_restore(flags);
}
