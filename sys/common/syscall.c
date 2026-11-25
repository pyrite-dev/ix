#include <sys/syscall.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/interrupt.h>
#include <sys/thread.h>
#include <machine/syscall.h>

#include <stddef.h>

void syscall_init(void) {
#ifdef SYSCALL_VECTOR
	_intr_vector(SYSCALL_VECTOR, _syscall_entry);
#endif
}

static int syscall_call(struct syscall_args* args) {
	switch(args->number) {
	case SYS_syscall:
	{
		struct syscall_args inner;

		inner.number = args->arg1;
		inner.arg1   = args->arg2;
		inner.arg2   = args->arg3;
		inner.arg3   = args->arg4;
		inner.arg4   = args->arg5;
		inner.arg5   = 0;

		return syscall_call(&inner);
	}
	case SYS_exit:
		thread_exit();
		return 0;
	case SYS_fork:
		return -ENOSYS;
	case SYS_read:
		return sys_read((int)args->arg1, (void*)args->arg2, (size_t)args->arg3);
	case SYS_write:
		return sys_write((int)args->arg1, (const void*)args->arg2, (size_t)args->arg3);
	case SYS_open:
		return sys_open((const char*)args->arg1, (int)args->arg2);
	case SYS_close:
		return sys_close((int)args->arg1);
	default:
		return -ENOSYS;
	}
}

void syscall_dispatch(struct iregs* iregs) {
	struct syscall_args args;

	_syscall_args(iregs, &args);

	_syscall_return(iregs, (reg_t)syscall_call(&args));
}
