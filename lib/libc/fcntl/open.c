#include <fcntl.h>
#include <errno.h>

#include <sys/syscall.h>

int open(const char* pathname, int flags) {
	int fd = _syscall(SYS_open, (reg_t)pathname, flags, 0, 0, 0);

	if(fd < 0) {
		errno = -fd;
		return -1;
	}

	return fd;
}
