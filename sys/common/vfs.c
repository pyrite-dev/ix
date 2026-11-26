#include <sys/vfs.h>
#include <sys/devfs.h>
#include <sys/dirent.h>
#include <sys/errno.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int rootfs_lookup(struct vnode* dir, const char* name, struct vnode** result);
static int rootfs_readdir(struct vnode* dir, off_t index, unsigned int* ino, char* name, size_t name_size);

static const struct vnode_ops rootfs_ops = {
    rootfs_lookup,
    rootfs_readdir};

static struct vnode* rootfs_root;
static struct vnode* rootfs_dev;

static struct mount* mounts;

struct vnode* vnode_alloc(int type, const struct vnode_ops* ops, struct mount* mount) {
	struct vnode* vnode = malloc(sizeof(*vnode));

	if(vnode == NULL) return NULL;

	memset(vnode, 0, sizeof(*vnode));
	vnode->type  = type;
	vnode->ops   = ops;
	vnode->mount = mount;
	vnode->ref   = 1;

	return vnode;
}

void vnode_get(struct vnode* vnode) {
	vnode->ref++;
}

void vnode_put(struct vnode* vnode) {
	if(--vnode->ref == 0) free(vnode);
}

static int rootfs_lookup(struct vnode* dir, const char* name, struct vnode** result) {
	if(dir != rootfs_root) return ENOENT;

	if(strcmp(name, "dev") != 0) return ENOENT;

	vnode_get(rootfs_dev);

	*result = rootfs_dev;

	return 0;
}

static int rootfs_readdir(struct vnode* dir, off_t index, unsigned int* ino, char* name, size_t name_size) {
	const char*  entry;
	unsigned int entry_ino;

	if(dir != rootfs_root) return ENOENT;

	switch(index) {
	case 0:
		entry	  = ".";
		entry_ino = 1;
		break;
	case 1:
		entry	  = "..";
		entry_ino = 1;
		break;
	case 2:
		entry	  = "dev";
		entry_ino = 2;
		break;
	default:
		return ENOENT;
	}

	if(strlen(entry) + 1 > name_size) return ENAMETOOLONG;

	strcpy(name, entry);
	*ino = entry_ino;

	return 0;
}

void vfs_init(void) {
	static struct mount devfs_mount;

	rootfs_root = vnode_alloc(VNODE_DIR, &rootfs_ops, NULL);
	rootfs_dev  = vnode_alloc(VNODE_DIR, &rootfs_ops, NULL);

	devfs_init(&devfs_mount);
	devfs_mount.point = rootfs_dev;
	devfs_mount.next  = mounts;

	mounts = &devfs_mount;
}

static struct vnode* vfs_cross_mount(struct vnode* vnode) {
	struct mount* mount;

	for(mount = mounts; mount != NULL; mount = mount->next) {
		if(mount->point == vnode) {
			vnode_get(mount->root);
			vnode_put(vnode);

			return mount->root;
		}
	}

	return vnode;
}

#define VFS_LOOKUP_MAX_DEPTH 64

int vfs_lookup(const char* path, struct vnode** result) {
	struct vnode* parents[VFS_LOOKUP_MAX_DEPTH];
	struct vnode* current;
	const char*   p;
	int	      depth = 0;
	int	      error;

	if(path[0] != '/') return ENOENT;

	current = rootfs_root;
	vnode_get(current);

	p = path;

	while(*p != '\0') {
		const char*   start;
		size_t	      length;
		char	      name[DIRENT_NAME_MAX];
		struct vnode* next;

		while(*p == '/') p++;
		if(*p == '\0') break;

		start = p;

		while(*p != '\0' && *p != '/') p++;

		length = (size_t)(p - start);

		if(length >= DIRENT_NAME_MAX) {
			error = ENAMETOOLONG;
			goto out;
		}

		memcpy(name, start, length);
		name[length] = '\0';

		current = vfs_cross_mount(current);

		if(current->type != VNODE_DIR) {
			error = ENOTDIR;
			goto out;
		}

		if(strcmp(name, ".") == 0) continue;

		if(strcmp(name, "..") == 0) {
			if(depth > 0) {
				vnode_put(current);
				current = parents[--depth];
			}

			continue;
		}

		if(depth >= VFS_LOOKUP_MAX_DEPTH) {
			error = ENAMETOOLONG;
			goto out;
		}

		error = current->ops->lookup(current, name, &next);
		if(error != 0) goto out;

		parents[depth++] = current;
		current		 = next;
	}

	current = vfs_cross_mount(current);

	*result = current;

	return 0;

out:
	vnode_put(current);

	while(depth > 0) vnode_put(parents[--depth]);

	return error;
}
