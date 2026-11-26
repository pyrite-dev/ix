#ifndef __SYS_DIRENT_H__
#define __SYS_DIRENT_H__

struct dirent;

#define DIRENT_NAME_MAX 256

#include <sys/types.h>

struct dirent {
	ino_t d_ino;
	char  d_name[DIRENT_NAME_MAX];
};

#endif
