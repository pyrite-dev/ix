#ifndef __SYS_VFS_H__
#define __SYS_VFS_H__

struct vnode;
struct vnode_ops;
union vnode_data;
struct mount;

enum vtype {
	VNODE_NONE = 0,
	VNODE_DIR,
	VNODE_CHR,
	VNODE_BLK,
	VNODE_REG
};

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stddef.h>

union vnode_data {
	dev_t dev;
	void* ptr;
};

struct vnode {
	int			type;
	const struct vnode_ops* ops;
	struct mount*		mount;
	union vnode_data	data;
	unsigned int		ref;
};

struct vnode_ops {
	int (*lookup)(struct vnode* dir, const char* name, struct vnode** result);
	int (*readdir)(struct vnode* dir, off_t index, unsigned int* ino, char* name, size_t name_size);
};

struct mount {
	struct vnode* root;
	struct vnode* point;
	struct mount* next;
};

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void vfs_init(void);
int  vfs_lookup(const char* path, struct vnode** result);

struct vnode* vnode_alloc(int type, const struct vnode_ops* ops, struct mount* mount);
void	      vnode_get(struct vnode* vnode);
void	      vnode_put(struct vnode* vnode);
#endif

__END_EXTERN_C__

#endif
