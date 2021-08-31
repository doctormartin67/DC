#ifndef LIBRARY_HEADER
#define LIBRARY_HEADER

#include <ctype.h>
#include <stddef.h>

#ifdef _WINDOWS
#define strcasecmp stricmp
#endif

#define GARBAGE " \r\n\t:"

char *strinside(const char *s, const char *restrict begin,
		const char *restrict end);
char *replace(const char *str, const char *orig, const char *rep);
int DIRexists(const char *restrict dname);
double min(int, ...);
double max(int, ...);
double sum(size_t length, double a[restrict length]);
void *jalloc(size_t n, size_t size);

/*
 * inline functions
 */
inline int isgarbage(int c) 
{
	const char *s = GARBAGE;

	while (*s)
		if (c == *s++)
			return 1;
	return 0;
}

inline char *trim(char s[static 1])
{
	char *t = s;
	while (isgarbage(*s)) s++;
	t = s;
	while (*s) s++;
	s--;
	while (isgarbage(*s)) *s-- = '\0';

	return t;
}

inline void upper(char s[static restrict 1])
{
	while(*s) {
		*s = toupper(*s);
		s++;
	}
}

inline int isfloat(const char s[static restrict 1])
{
	while (isgarbage(*s)) s++;    

	if(*s == '+' || *s == '-') s++;

	if (!isdigit(*s)) return 0;

	while (isdigit(*s)) s++;

	if (*s == '\0')
		return 1;
	else if (*s == '.')
		s++;
	else
		return 0;

	while (isdigit(*s)) s++;

	if (*s == '\0')
		return 1;
	else
		return 0;
}

inline int isint(const char s[static restrict 1])
{
	while (isgarbage(*s)) s++;    

	if(*s == '+' || *s == '-') s++;

	if (!isdigit(*s)) return 0;

	while (isdigit(*s)) s++;

	if (*s == '\0')
		return 1;
	else
		return 0;
}

#endif
