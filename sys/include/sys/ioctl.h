#ifndef __SYS_IOCTL_H__
#define __SYS_IOCTL_H__

#define IOC_DIRMASK 0xe0000000UL
#define IOC_DIRSHIFT 29

#define IOC_PLENMASK 0x1fffUL
#define IOC_PLENSHIFT 16

#define IOC_GRPMASK 0xffUL
#define IOC_GRPSHIFT 8

#define IOC_NUMMASK 0xffUL
#define IOC_NUMSHIFT 1

#define IOC_VOID 0x20000000UL
#define IOC_IN 0x40000000UL  /* Userland -> Kernel */
#define IOC_OUT 0x80000000UL /* Kernel -> Userland */
#define IOC_INOUT (IOC_IN | IOC_OUT)

#define IOC_DIR(cmd) ((cmd) & IOC_DIRMASK)
#define IOC_PLEN(cmd) (((cmd) >> IOC_PLENSHIFT) & IOC_PLENMASK)
#define IOC_GRP(cmd) (((cmd) >> IOC_GRPSHIFT) & IOC_GRPMASK)
#define IOC_NUM(cmd) (((cmd) >> IOC_NUMSHIFT) & IOC_NUMMASK)

#define _IOC(inout, group, num, plen) ((unsigned long)(inout) | (((unsigned long)(plen) & IOC_PLENMASK) << IOC_PLENSHIFT) | (((unsigned long)(group) & IOC_GRPMASK) << IOC_GRPSHIFT) | (((unsigned long)(num) & IOC_NUMMASK) << IOC_NUMSHIFT))

#define _IO(group, num) _IOC(IOC_VOID, group, num, 0)
#define _IOR(group, num, type) _IOC(IOC_OUT, group, num, sizeof(type))
#define _IOW(group, num, type) _IOC(IOC_IN, group, num, sizeof(type))
#define _IOWR(group, num, type) _IOC(IOC_INOUT, group, num, sizeof(type))

#define TIOCGETA _IOR('t', 1, struct termios)

#define DIOCGSIZE _IOR('d', 1, off_t)
#define TIOCSETA _IOW('t', 2, struct termios)
#define TIOCSETAW _IOW('t', 3, struct termios)
#define TIOCSETAF _IOW('t', 4, struct termios)

#include <sys/termios.h>
#include <sys/types.h>
#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

int ioctl(int fd, unsigned long cmd, ...);

__END_EXTERN_C__

#endif
