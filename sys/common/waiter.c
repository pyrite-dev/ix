#include <sys/waiter.h>
#include <sys/spinlock.h>
#include <sys/thread.h>
#include <sys/interrupt.h>

#include <stddef.h>

void waiter_init(waiter_t* waiter) {
	spinlock_init(&waiter->lock);

	waiter->head = NULL;
	waiter->tail = NULL;
}

void waiter_sleep(waiter_t* waiter) {
	thread_t*    thread = thread_current();
	intr_flags_t flags  = spinlock_lock_irqsave(&waiter->lock);

	thread->waiting = waiter;
	thread->state	= THREAD_WAITING;

	spinlock_unlock_irqrestore(&waiter->lock, flags);
}

void waiter_wakeup(waiter_t* waiter) {
}

void waiter_wakeup_one(waiter_t* waiter) {
}
