#ifndef __SYS_THREAD_H__
#define __SYS_THREAD_H__

typedef struct thread thread_t;

struct thread;

enum THREAD_STATE {
	THREAD_READY = 0,
	THREAD_RUNNING,
	THREAD_WAITING,
	THREAD_DEAD
};

#include <sys/cdefs.h>
#include <sys/waiter.h>
#include <machine/thread.h>

#include <stddef.h>

struct thread {
	int state;

	waiter_t* waiting;
	thread_t* wait_prev;
	thread_t* wait_next;

	thread_t* sched_prev;
	thread_t* sched_next;

	void*  stack_base;
	size_t stack_size;

	void (*entry)(void*);
	void* arg;

	unsigned int preempt;
	unsigned int quantum;

	thread_context_t context;
};

__BEGIN_EXTERN_C__

thread_t* thread_current(void);

int thread_init(thread_t* thread, void* stack, size_t stack_size, void (*entry)(void*), void* arg);

void thread_exit(void);

int _thread_init(thread_t* thread);

__END_EXTERN_C__

#endif
