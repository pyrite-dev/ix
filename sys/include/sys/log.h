#ifndef __SYS_LOG_H__
#define __SYS_LOG_H__

#include <sys/cdefs.h>
#include <sys/syslog.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void log(int level, const char* format, ...);
#endif

__END_EXTERN_C__

#endif
