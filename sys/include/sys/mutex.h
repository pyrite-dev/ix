#ifndef __SYS_MUTEX_H__
#define __SYS_MUTEX_H__

typedef struct mutex mutex_t;

struct mutex;

#include <sys/cdefs.h>
#include <sys/spinlock.h>
#include <sys/waiter.h>

struct mutex {
	spinlock_t lock;
	waiter_t   waiter;

	int cond;
};

__BEGIN_EXTERN_C__

void mutex_init(mutex_t* mutex);

void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);

__END_EXTERN_C__

#endif
