#include <string.h>

void* memcpy(void* s1, const void* s2, size_t n) {
	unsigned char*	     d = s1;
	const unsigned char* s = s2;

	while(n--) *d++ = *s++;

	return s1;
}
