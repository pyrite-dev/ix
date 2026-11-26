#include <stddef.h>

#define DIR_ENTRIES 8

struct __dir {
	int	      fd;
	size_t	      count;
	size_t	      pos;
	struct dirent entries[DIR_ENTRIES];
};
