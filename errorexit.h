#ifndef ERROREXIT
#define ERROREXIT

void errExit(const char *restrict format, ...);
void errExitEN(int errnum, const char *restrict format, ...);

#endif
