#ifndef __SYS_OSINFO_H__
#define __SYS_OSINFO_H__

#include <sys/cdefs.h>

#define OS_VERSION "1.0"

__BEGIN_EXTERN_C__

extern const char* os_ident;
extern const char* os_date;
extern const char* os_version;

__END_EXTERN_C__

#endif
