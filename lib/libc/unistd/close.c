#include <unistd.h>
#include <errno.h>

#include <sys/syscall.h>

int close(int fd) {
	int err = _syscall(SYS_close, fd, 0, 0, 0, 0);

	if(err < 0) {
		errno = -err;
		return -1;
	}

	return err;
}
