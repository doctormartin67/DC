#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "common.h"
#include "validation.h"
#include "errorexit.h"

struct validator {
	char *error;
	unsigned num_errors;
};

static struct validator validator;

void validate_emit_error(const char *fmt, ...)
{
	char buf[BUFSIZ];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	buf_printf(validator.error, "%s\n", buf);
	va_end(args);
	validator.num_errors++;
}

void validate_reset(void)
{
	buf_free(validator.error);
	validator.num_errors = 0;
}

unsigned validate_passed(void)
{
	return !validator.num_errors;
}

const char *validate_error(void)
{
	return validator.error;
}
