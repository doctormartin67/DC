/*
 * defines exit functions with option to include error message using printf
 * format
 */
#ifndef ERROREXIT_H_
#define ERROREXIT_H_

/*
 * Stringification kicks in before argument replacement, below is a simple
 * trick to resolve this issue. It was needed for the macro __LINE__
 */
#define STRINGIFY(X) #X
#define STRGY(X) STRINGIFY(X)

/*
 * helper macro's for die macro. SING_ARGS takes the first argument,
 * PLURAL_ARGS takes all the rest
 */
#define SING_ARGS_0(_0, ...) _0
#define SING_ARGS(...) SING_ARGS_0(__VA_ARGS__, 0)
#define PLURAL_ARGS_0(_0, ...) __VA_ARGS__
#define PLURAL_ARGS(...) PLURAL_ARGS_0(__VA_ARGS__, 0)

void die(const char *restrict format, ...)
	__attribute__ ((format (printf, 1, 2)));
void errExitEN(int errnum, const char *restrict format, ...)
	__attribute__ ((format (printf, 2, 3)));

#define ERREXIT(F, ...) die("%s:" STRGY(__LINE__) ": " F, \
		__func__, __VA_ARGS__);
#define die(...) ERREXIT(SING_ARGS(__VA_ARGS__) "%.0d", \
		PLURAL_ARGS(__VA_ARGS__))

#endif
