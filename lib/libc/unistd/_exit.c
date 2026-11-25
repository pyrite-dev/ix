#include <unistd.h>

#include <sys/syscall.h>

void _exit(int status) {
	_syscall(SYS_exit, status, 0, 0, 0, 0);
}
