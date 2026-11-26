#include <dirent.h>
#include <stddef.h>
#include <errno.h>

#include <sys/syscall.h>

#include "_dirent.h"

static int getdents(int fd, void* buffer, size_t count) {
	int r = _syscall(SYS_getdents, fd, (reg_t)buffer, count, 0, 0);

	if(r < 0) {
		errno = -r;
		return -1;
	}

	return r;
}

struct dirent* readdir(DIR* dir) {
	int r;

	if(dir->pos >= dir->count) {
		r = getdents(dir->fd, dir->entries, sizeof(dir->entries));
		if(r <= 0) return NULL;

		dir->count = (size_t)r / sizeof(struct dirent);
		dir->pos   = 0;
	}

	return &dir->entries[dir->pos++];
}
