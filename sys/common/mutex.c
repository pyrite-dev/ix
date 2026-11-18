#include <sys/mutex.h>
#include <sys/spinlock.h>
#include <sys/waiter.h>

void mutex_init(mutex_t* mutex) {
	spinlock_init(&mutex->lock);
	waiter_init(&mutex->waiter);
	mutex->cond = 1;
}

void mutex_lock(mutex_t* mutex) {
	intr_flags_t flags = spinlock_lock_irqsave(&mutex->lock);
	while(!mutex->cond) waiter_sleep(&mutex->waiter, &mutex->lock, flags);
	mutex->cond = 0;
	spinlock_unlock_irqrestore(&mutex->lock, flags);
}

void mutex_unlock(mutex_t* mutex) {
	intr_flags_t flags = spinlock_lock_irqsave(&mutex->lock);
	mutex->cond	   = 1;
	waiter_wakeup_one(&mutex->waiter);
	spinlock_unlock_irqrestore(&mutex->lock, flags);
}
