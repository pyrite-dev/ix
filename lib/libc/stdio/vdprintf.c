#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

struct vdprintf_state {
	int  fd;
	char buf[128];
	int  len;
	int  err;
};

static void flush(struct vdprintf_state* state) {
	int done = 0;

	while(!state->err && done < state->len) {
		int r = write(state->fd, state->buf + done, state->len - done);

		if(r <= 0) {
			state->err = 1;
			break;
		}

		done += r;
	}

	state->len = 0;
}

static void helper(struct vdprintf_state* state, char c) {
	state->buf[state->len++] = c;

	if(state->len == sizeof(state->buf)) flush(state);
}

#define STATIC
#define FUNCNAME _vdprintf
#define ARG1 state
#define ARG1TYPE struct vdprintf_state*
#include "_vprintf.h"

int vdprintf(int fd, const char* format, va_list ap) {
	struct vdprintf_state state;
	int		      r;

	state.fd  = fd;
	state.len = 0;
	state.err = 0;

	r = _vdprintf(&state, format, ap);

	flush(&state);

	return state.err ? -1 : r;
}
