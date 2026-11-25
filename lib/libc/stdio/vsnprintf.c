#include <stdio.h>
#include <stdarg.h>

struct vsnprintf_state {
	char*  str;
	size_t size;
	size_t len;
};

static void helper(struct vsnprintf_state* state, char c) {
	if(state->len + 1 < state->size) state->str[state->len] = c;

	state->len++;
}

#define FUNCNAME _vsnprintf
#define ARG1 state
#define ARG1TYPE struct vsnprintf_state*
#include "_vprintf.h"

int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
	struct vsnprintf_state state;
	int		       r;

	state.str  = str;
	state.size = size;
	state.len  = 0;

	r = _vsnprintf(&state, format, ap);

	if(size != 0) str[state.len < size ? state.len : size - 1] = 0;

	return r;
}
