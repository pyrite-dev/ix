#include <string.h>

void* memset(void* dst, int c, size_t n){
	char* d = (char*)dst;

	while(n--) *d++ = c;

	return dst;
}
