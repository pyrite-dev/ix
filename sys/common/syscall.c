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
	iregs->g0 = ENOSYS;
}
