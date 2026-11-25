#ifndef __SYS_SYSCALL_H__
#define __SYS_SYSCALL_H__

struct syscall_args;

#include <sys/cdefs.h>
#include <machine/regs.h>

struct syscall_args {
	reg_t number;
	reg_t arg1;
	reg_t arg2;
	reg_t arg3;
	reg_t arg4;
	reg_t arg5;
};

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void syscall_init(void);

void syscall_dispatch(struct iregs* iregs);

reg_t _syscall_entry(reg_t number, reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);
reg_t _syscall_invoke(reg_t number, reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);

void _syscall_args(struct iregs* iregs, struct syscall_args* args);
void _syscall_return(struct iregs* iregs, reg_t v);
#endif

__END_EXTERN_C__

#endif
