#include <stdlib.h>

libc_malloc_t libc_malloc = NULL;

void* malloc(size_t size) {
	if(libc_malloc == NULL) {
		return NULL;
	} else {
		return libc_malloc(size);
	}
}
