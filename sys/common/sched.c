#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/interrupt.h>

#include <stdlib.h>
#include <string.h>

static char idle_stack[512];

static void idle_entry(void* arg) {
	/* TODO: Do not use busy loop here!!! */

	while(1);
}

static thread_t* current_thread;
static thread_t* ready_head;
static thread_t* ready_tail;
static thread_t* idle_thread;

static unsigned int resched_pending;
int		    sched_started = 0;

thread_t* thread_current(void) {
	return current_thread;
}

int thread_init(thread_t* thread, void* stack, size_t stack_size, void (*entry)(void*), void* arg) {
	thread->state = THREAD_WAITING;

	thread->waiting	  = NULL;
	thread->wait_prev = NULL;
	thread->wait_next = NULL;

	thread->sched_prev = NULL;
	thread->sched_next = NULL;

	thread->stack_base = stack;
	thread->stack_size = stack_size;

	thread->entry = entry;
	thread->arg   = arg;

	thread->preempt = 0;
	thread->quantum = SCHED_QUANTUM;

	memset(&thread->context, 0, sizeof(thread->context));

	if(_thread_init(thread) != 0) return -1;

	return 0;
}

void thread_exit(void) {
	thread_t*    thread = current_thread;
	intr_flags_t flags;

	flags = _intr_save();

	thread->state = THREAD_DEAD;

	sched_block(flags);

	while(1);
}
void thread_start(thread_t* thread) {
	thread->entry(thread->arg);
	thread_exit();

	while(1);
}

static void ready_insert(thread_t* thread) {
	thread->sched_prev = ready_tail;
	thread->sched_next = NULL;

	if(ready_tail == NULL) {
		ready_head = thread;
	} else {
		ready_tail->sched_next = thread;
	}

	ready_tail = thread;
}

static void ready_remove(thread_t* thread) {
	if(thread->sched_prev == NULL) {
		ready_head = thread->sched_next;
	} else {
		thread->sched_prev->sched_next = thread->sched_next;
	}

	if(thread->sched_next == NULL) {
		ready_tail = thread->sched_prev;
	} else {
		thread->sched_next->sched_prev = thread->sched_prev;
	}

	thread->sched_prev = NULL;
	thread->sched_next = NULL;
}

static thread_t* ready_get(void) {
	thread_t* thread;

	thread = ready_head;

	if(thread != NULL) ready_remove(thread);

	return thread;
}

void sched_init(void) {
	current_thread = NULL;
	ready_head     = NULL;
	ready_tail     = NULL;

	idle_thread = malloc(sizeof(*idle_thread));
	thread_init(idle_thread, idle_stack, sizeof(idle_stack), idle_entry, NULL);

	resched_pending = 0;
	sched_started	= 0;

	idle_thread->state	= THREAD_READY;
	idle_thread->quantum	= SCHED_QUANTUM;
	idle_thread->preempt	= 0;
	idle_thread->sched_prev = NULL;
	idle_thread->sched_next = NULL;
}

void sched_start(void) {
	intr_flags_t flags = _intr_save();

	sched_started = 1;

	_intr_restore(flags);

	while(1);
}

void sched_add(thread_t* thread) {
	intr_flags_t flags;

	flags = _intr_save();

	if(thread->state == THREAD_DEAD || thread->state == THREAD_READY || thread->state == THREAD_RUNNING) {
		_intr_restore(flags);
		return;
	}

	thread->state	= THREAD_READY;
	thread->quantum = SCHED_QUANTUM;

	thread->sched_prev = NULL;
	thread->sched_next = NULL;

	ready_insert(thread);

	_intr_restore(flags);
}

void sched_remove(thread_t* thread) {
	intr_flags_t flags;

	flags = _intr_save();

	if(thread->state == THREAD_READY) {
		ready_remove(thread);
		thread->state = THREAD_WAITING;
	}

	_intr_restore(flags);
}

static thread_t* sched_choose(void) {
	thread_t* next;

	next = ready_get();

	if(next == NULL) next = idle_thread;

	return next;
}

void sched_tick(void) {
	thread_t* thread;

	thread = current_thread;

	if(thread == NULL) return;

	if(thread == idle_thread) {
		if(ready_head != NULL) resched_pending = 1;
		return;
	}

	if(thread->quantum > 0) thread->quantum--;

	if(thread->quantum == 0) resched_pending = 1;
}

void sched_preempt(thread_context_t* context) {
	thread_t* old;
	thread_t* next;

	old = current_thread;

	if(old == NULL) {
		next = sched_choose();

		next->state   = THREAD_RUNNING;
		next->quantum = SCHED_QUANTUM;

		current_thread	= next;
		resched_pending = 0;

		*context = next->context;
		return;
	}

	if(old->preempt != 0) return;

	if(!resched_pending) return;

	_sched_save(&old->context, context);

	if(old->state == THREAD_RUNNING) {
		old->state   = THREAD_READY;
		old->quantum = SCHED_QUANTUM;

		if(old != idle_thread) ready_insert(old);
	}

	next = sched_choose();

	if(next == old) {
		old->state	= THREAD_RUNNING;
		old->quantum	= SCHED_QUANTUM;
		resched_pending = 0;
		return;
	}

	next->state	= THREAD_RUNNING;
	next->quantum	= SCHED_QUANTUM;
	current_thread	= next;
	resched_pending = 0;

	_sched_restore(context, &next->context);
}

void sched_block(intr_flags_t flags) {
	thread_t* thread = current_thread;

	resched_pending = 1;

	_intr_restore(flags);

	while(thread->state != THREAD_RUNNING) __BARRIER__();
}

void sched_wakeup(thread_t* thread) {
	intr_flags_t flags;

	flags = _intr_save();

	if(thread->state == THREAD_WAITING) {
		thread->state	= THREAD_READY;
		thread->quantum = SCHED_QUANTUM;

		thread->sched_prev = NULL;
		thread->sched_next = NULL;

		ready_insert(thread);
	}

	_intr_restore(flags);
}

void sched_preempt_disable(void) {
	thread_t* thread;

	thread = current_thread;

	if(thread != NULL) thread->preempt++;
}

void sched_preempt_enable(void) {
	thread_t* thread;

	thread = current_thread;

	if(thread == NULL) return;

	if(thread->preempt > 0) thread->preempt--;
}
