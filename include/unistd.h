#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/unistd.h>

#include <stddef.h>

__BEGIN_EXTERN_C__

void _exit(int status);

int close(int fd);

ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, const void* buf, size_t count);
off_t	lseek(int fd, off_t offset, int whence);

__END_EXTERN_C__

#endif
