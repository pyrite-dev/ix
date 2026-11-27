#include <stdio.h>
#include <stdarg.h>

static void helper(int fd, char c) {
	write(fd, &c, 1);
}

#define FUNCNAME vdprintf
#define ARG1 fd
#define ARG1TYPE int
#include "_vprintf.h"
