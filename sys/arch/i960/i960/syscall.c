#include <sys/syscall.h>

void _syscall_args(struct iregs* iregs, struct syscall_args* args) {
	args->number = iregs->g0;
	args->arg1   = iregs->g1;
	args->arg2   = iregs->g2;
	args->arg3   = iregs->g3;
	args->arg4   = iregs->g4;
	args->arg5   = iregs->g5;
}

void _syscall_return(struct iregs* iregs, reg_t v) {
	iregs->g0 = v;
}
