#include <stdlib.h>

libc_realloc_t libc_realloc = NULL;

void* realloc(void* ptr, size_t size) {
	if(libc_realloc == NULL) {
		return NULL;
	} else {
		return libc_realloc(ptr, size);
	}
}
