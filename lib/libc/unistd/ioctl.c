#include <stdarg.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/syscall.h>

int ioctl(int fd, unsigned long cmd, ...) {
	va_list ap;
	void*	arg;
	int	r;

	va_start(ap, cmd);
	arg = va_arg(ap, void*);
	va_end(ap);

	r = _syscall(SYS_ioctl, fd, (reg_t)cmd, (reg_t)arg, 0, 0);

	if(r < 0) {
		errno = -r;
		return -1;
	}

	return r;
}
