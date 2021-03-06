/*
 * error diagnostic functions
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

static void outputError(int useErr, int err, int flushStdout, 
		const char *restrict format, va_list ap)
{
	char buf[BUFSIZ * 4], userMsg[BUFSIZ], errText[BUFSIZ];

	vsnprintf(userMsg, BUFSIZ, format, ap);

	if (useErr)
		snprintf(errText, sizeof(errText), " [%s]", strerror(err));
	else
		snprintf(errText, sizeof(errText), ":");

	snprintf(buf, sizeof(buf), "ERROR%s | %s\n", errText, userMsg);

	if (flushStdout)
		fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
}

_Noreturn void die(const char *restrict format, ...)
{
	va_list arglist;

	va_start(arglist, format);
	outputError(1, errno, 1, format, arglist);
	va_end(arglist);

	exit(EXIT_FAILURE);
}
