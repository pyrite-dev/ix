#include <string.h>

void* memmove(void* s1, const void* s2, size_t n) {
	unsigned char*	     p1 = s1;
	const unsigned char* p2 = s2;

	if(p1 <= p2) {
		memcpy(p1, p2, n);
	} else {
		p1 += n;
		p2 += n;

		while(n--) *--p1 = *--p2;
	}

	return s1;
}
