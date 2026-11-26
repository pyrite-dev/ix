#ifndef __SYS_DEVFS_H__
#define __SYS_DEVFS_H__

#include <sys/cdefs.h>
#include <sys/vfs.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
int devfs_init(struct mount* mount);
#endif

__END_EXTERN_C__

#endif
