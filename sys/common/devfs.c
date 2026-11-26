#include <sys/devfs.h>
#include <sys/bdev.h>
#include <sys/cdev.h>
#include <sys/errno.h>
#include <sys/vfs.h>

#include <stddef.h>
#include <string.h>

static int devfs_lookup(struct vnode* dir, const char* name, struct vnode** result);
static int devfs_readdir(struct vnode* dir, off_t index, unsigned int* ino, char* name, size_t name_size);

static const struct vnode_ops devfs_ops = {
    devfs_lookup,
    devfs_readdir};

int devfs_init(struct mount* mount) {
	struct vnode* vnode = vnode_alloc(VNODE_DIR, &devfs_ops, mount);

	if(vnode == NULL) return ENOMEM;

	mount->root = vnode;

	return 0;
}

static int devfs_lookup(struct vnode* dir, const char* name, struct vnode** result) {
	struct vnode* vnode;
	dev_t	      dev;
	enum vtype    type;

	if(dir->type != VNODE_DIR || dir->ops != &devfs_ops) return ENOENT;

	if(cdev_find_device(name, &dev) == 0) {
		type = VNODE_CHR;
	} else if(bdev_find_device(name, &dev) == 0) {
		type = VNODE_BLK;
	} else {
		return ENOENT;
	}

	vnode = vnode_alloc(type, &devfs_ops, dir->mount);
	if(vnode == NULL) return ENOMEM;

	vnode->data.dev = dev;

	*result = vnode;

	return 0;
}

static int devfs_readdir(struct vnode* dir, off_t index, unsigned int* ino, char* name, size_t name_size) {
	unsigned int count;
	dev_t	     dev;
	int	     error;

	if(dir->type != VNODE_DIR || dir->ops != &devfs_ops) return ENOENT;

	switch(index) {
	case 0:
		strcpy(name, ".");
		*ino = 2;
		return 0;
	case 1:
		strcpy(name, "..");
		*ino = 1;
		return 0;
	default:
		break;
	}

	index -= 2;

	count = cdev_device_count();

	if(index < count) {
		error = cdev_device_at(index, &dev, name, name_size);
	} else {
		error = bdev_device_at(index - count, &dev, name, name_size);
	}

	if(error != 0) return ENOENT;

	*ino = dev;

	return 0;
}
