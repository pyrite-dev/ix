#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "_dirent.h"

DIR* opendir(const char* path) {
	DIR* dir;
	int  fd;

	fd = open(path, O_RDONLY);
	if(fd < 0) return NULL;

	dir = malloc(sizeof(*dir));
	if(dir == NULL) {
		close(fd);
		errno = ENOMEM;
		return NULL;
	}

	dir->fd	   = fd;
	dir->count = 0;
	dir->pos   = 0;

	return dir;
}
