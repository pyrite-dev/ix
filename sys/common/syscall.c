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

	kprintf("syscall: %d %d %d %d %d %d\r\n", args.number, args.arg1, args.arg2, args.arg3, args.arg4, args.arg5);

	_syscall_return(iregs, ENOSYS);
}
