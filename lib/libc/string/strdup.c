#include <string.h>
#include <stdlib.h>

char* strdup(const char* s) {
	char* r = malloc(strlen(s) + 1);

	if(r == NULL) return NULL;

	strcpy(r, s);

	return r;
}
