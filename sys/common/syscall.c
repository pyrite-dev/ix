#include <sys/syscall.h>
#include <sys/interrupt.h>
#include <sys/errno.h>
#include <machine/syscall.h>

void syscall_init(void) {
	_intr_set(SYSCALL_VECTOR, _syscall_stub);
}

void syscall_dispatch(struct iregs* iregs) {
	iregs->g0 = ENOSYS;
}
