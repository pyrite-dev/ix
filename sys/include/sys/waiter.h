#ifndef __SYS_WAITER_H__
#define __SYS_WAITER_H__

typedef struct waiter waiter_t;

struct waiter;

#include <sys/cdefs.h>
#include <sys/spinlock.h>
#include <sys/thread.h>

struct waiter {
	spinlock_t lock;

	thread_t* head;
	thread_t* tail;
};

__BEGIN_EXTERN_C__

void waiter_init(waiter_t* waiter);

void waiter_sleep(waiter_t* waiter);

void waiter_wakeup(waiter_t* waiter);
void waiter_wakeup_one(waiter_t* waiter);

__END_EXTERN_C__

#endif
