#ifndef ERRORJUMP_H_
#define ERRORJUMP_H_

#include <setjmp.h>

struct error {
	jmp_buf buf;
	char *str;
	unsigned is_error;
} error;

void reset_error(void);

#endif
