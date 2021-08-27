#ifndef LIBRARY_HEADER
#define LIBRARY_HEADER

#include <stddef.h>

#ifdef _WINDOWS
#define strcasecmp stricmp
#endif

#define GARBAGE " \r\n\t:"

char *trim(char *);
void upper(char *);
int isfloat(const char *);
int isint(const char *);
int isgarbage(int c);
char *strinside(const char *s, const char *begin, const char *end);
char *replace(const char *str, const char *orig, const char *rep);
int DIRexists(const char *dname);
double min(int, ...);
double max(int, ...);
double sum(double a[], int length); // sum all elements of array
void *jalloc(size_t n, size_t size);

#endif
