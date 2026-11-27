#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/syscall.h>

off_t lseek(int fd, off_t offset, int whence) {
	int r = _syscall(SYS_lseek, fd, offset, whence, 0, 0);

	if(r < 0) {
		errno = -r;
		return -1;
	}

	return r;
}
