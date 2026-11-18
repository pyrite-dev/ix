#ifndef __ERRNO_H__
#define __ERRNO_H__

#include <sys/cdefs.h>
#include <sys/errno.h>

__BEGIN_EXTERN_C__

extern int	   errno;
extern const char* sys_errlist[];
extern int	   sys_nerr;

__END_EXTERN_C__

#endif
