#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>

#include "_dirent.h"

int closedir(DIR* dir) {
	int r = close(dir->fd);

	free(dir);

	return r;
}
