#include <sys/thread.h>

#include <string.h>

#define THREAD_FRAME_SIZE 64

static void thread_start(thread_t* thread) {
	thread->entry(thread->arg);
	thread_exit();

	while(1);
}

int _thread_init(thread_t* thread) {
	unsigned int  base;
	unsigned int  limit;
	unsigned int  fp;
	unsigned int* frame;

	if(thread == NULL || thread->stack_base == NULL) {
		return -1;
	}

	base  = (unsigned int)thread->stack_base;
	limit = base + thread->stack_size;

	if(limit < base) {
		return -1;
	}

	fp = (base + 63) & ~63;

	if(fp < base || fp > limit || limit - fp < THREAD_FRAME_SIZE) {
		return -1;
	}

	frame = (unsigned int*)fp;

	memset(frame, 0, THREAD_FRAME_SIZE);
	frame[0] = fp;
	frame[1] = fp + THREAD_FRAME_SIZE;
	frame[2] = (unsigned int)thread_start;

	thread->context.g0  = (unsigned int)thread;
	thread->context.pc  = 2;
	thread->context.ac  = 0;
	thread->context.pfp = fp;

	return 0;
}
