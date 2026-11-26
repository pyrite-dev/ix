#ifndef __DIRENT_H__
#define __DIRENT_H__

#include <sys/cdefs.h>
#include <sys/dirent.h>

typedef struct __dir DIR;

__BEGIN_EXTERN_C__

DIR*	       opendir(const char* path);
struct dirent* readdir(DIR* dir);
int	       closedir(DIR* dir);

__END_EXTERN_C__

#endif
