#ifndef __SYS_SYSCALL_H__
#define __SYS_SYSCALL_H__

#include <sys/cdefs.h>
#include <machine/regs.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void syscall_init(void);

void syscall_dispatch(struct iregs* iregs);

unsigned int _syscall_entry(unsigned int number, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, unsigned int arg5);
unsigned int _syscall_invoke(unsigned int number, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, unsigned int arg5);
#endif

__END_EXTERN_C__

#endif
