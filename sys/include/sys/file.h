#ifndef __SYS_FILE_H__
#define __SYS_FILE_H__

struct file;

#define NFILE 32

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stddef.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void file_init(void);

int sys_open(const char* path, int flags);
int sys_close(int fd);
int sys_read(int fd, void* buffer, size_t count);
int sys_write(int fd, const void* buffer, size_t count);
int sys_getdents(int fd, void* buffer, size_t count);
#endif

__END_EXTERN_C__

#endif
