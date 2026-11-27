#ifndef __SYS_SYSCALL_H__
#define __SYS_SYSCALL_H__

struct syscall_args;

#define SYS_syscall 0
#define SYS_exit 1
#define SYS_fork 2
#define SYS_read 3
#define SYS_write 4
#define SYS_lseek 5
#define SYS_open 6
#define SYS_close 7
#define SYS_getdents 8

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

void _syscall_args(struct iregs* iregs, struct syscall_args* args);
void _syscall_return(struct iregs* iregs, reg_t v);
#endif

reg_t _syscall(reg_t number, reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);

__END_EXTERN_C__

#endif
