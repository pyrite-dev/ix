#ifndef __SYS_SPINLOCK_H__
#define __SYS_SPINLOCK_H__

typedef struct spinlock spinlock_t;

struct spinlock;

#include <sys/cdefs.h>
#include <machine/interrupt.h>

struct spinlock {
	volatile unsigned int lock;
};

__BEGIN_EXTERN_C__

void spinlock_init(spinlock_t* lock);

void spinlock_lock(spinlock_t* lock);
void spinlock_unlock(spinlock_t* lock);
int  spinlock_trylock(spinlock_t* lock);

intr_flags_t spinlock_lock_irqsave(spinlock_t* lock);
void	     spinlock_unlock_irqrestore(spinlock_t* lock, intr_flags_t flags);

unsigned int _spinlock_trylock(volatile unsigned int* lock);
void	     _spinlock_unlock(volatile unsigned int* lock);

__END_EXTERN_C__

#endif
