#include <unistd.h>
#include <errno.h>
#include <stddef.h>

#include <sys/types.h>
#include <sys/syscall.h>

ssize_t read(int fd, void* buf, size_t count) {
	int r = _syscall(SYS_read, fd, (reg_t)buf, count, 0, 0);

	if(r < 0) {
		errno = -r;
		return -1;
	}

	return r;
}
