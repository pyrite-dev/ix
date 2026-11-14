#include <stdlib.h>

libc_free_t libc_free = NULL;

void free(void* ptr) {
	if(libc_free == NULL) {
	} else {
		libc_free(ptr);
	}
}
