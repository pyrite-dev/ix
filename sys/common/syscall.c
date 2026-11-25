#include <sys/syscall.h>
#include <sys/errno.h>
#include <sys/interrupt.h>
#include <machine/syscall.h>

void syscall_init(void) {
#ifdef SYSCALL_VECTOR
	_intr_vector(SYSCALL_VECTOR, _syscall_entry);
#endif
}

void syscall_dispatch(struct iregs* iregs) {
	struct syscall_args args;

	_syscall_args(iregs, &args);

	switch(args.number) {
	case SYS_syscall:
	{
		_syscall_return(iregs, args.arg1);
		break;
	}
	defualt:
	{
		_syscall_return(iregs, ENOSYS);
		break;
	}
	}
}
