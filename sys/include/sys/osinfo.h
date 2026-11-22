#ifndef __SYS_OSINFO_H__
#define __SYS_OSINFO_H__

#define OS_VERSION "1.0"

#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
extern const char* os_ident;
extern const char* os_machine;
extern const char* os_date;
extern const char* os_version;
#endif

__END_EXTERN_C__

#endif
