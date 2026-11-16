#ifndef __MACHINE_SPINLOCK_H__
#define __MACHINE_SPINLOCK_H__

#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

unsigned int _spinlock_trylock(volatile unsigned int* lock);
void	     _spinlock_unlock(volatile unsigned int* lock);

__END_EXTERN_C__

#endif
