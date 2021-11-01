#ifndef LIBRARY_HEADER
#define LIBRARY_HEADER

#include <ctype.h>
#include <stddef.h>

#ifdef _WINDOWS
#define strcasecmp stricmp
#endif

#define GARBAGE " \n\t:\r"

#define MAX3(X, Y, Z) MAX2(MAX2(X, Y), Z)
#define MAX2(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN3(X, Y, Z) MIN2(MIN2(X, Y), Z)
#define MIN2(X, Y) ((X) < (Y) ? (X) : (Y))

char *strinside(const char *s, const char *restrict begin,
		const char *restrict end);
char *replace(const char *str, const char *orig, const char *rep);
int DIRexists(const char *restrict dname);
double sum(size_t length, double a[restrict length]);
void *jalloc(size_t n, size_t size);

/*
 * inline functions
 */
inline unsigned isgarbage(int c) 
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

inline void upper(char s[restrict static 1])
{
	while(*s) {
		*s = toupper(*s);
		s++;
	}
}

/*
 * returns 1 if s is a float, otherwhise returns 0
 */
inline unsigned isfloat(const char s[restrict static 1])
{
	while (isgarbage(*s)) s++;    

	if('+' == *s || '-' == *s) s++;

	if (!isdigit(*s)) return 0;

	while (isdigit(*s)) s++;

	if ('\0' == *s)
		return 1;
	else if ('.' == *s)
		s++;
	else
		return 0;

	while (isdigit(*s)) s++;
	while (isgarbage(*s)) s++;    

	if ('\0' == *s)
		return 1;
	else
		return 0;
}

/*
 * returns 1 if s is an integer, otherwhise returns 0
 */
inline unsigned isint(const char s[restrict static 1])
{
	while (isgarbage(*s)) s++;    

	if('+' == *s || '-' == *s) s++;

	if (!isdigit(*s)) return 0;

	while (isdigit(*s)) s++;
	while (isgarbage(*s)) s++;    

	if ('\0' == *s)
		return 1;
	else
		return 0;
}

/*
 * returns 1 of s is a float up until the character c or '\0' is reached,
 * otherwhise returns 0
 */
inline unsigned isfloatc(const char s[restrict static 1], int c)
{
	while (isgarbage(*s)) s++;    

	if('+' == *s || '-' == *s) s++;

	if (!isdigit(*s)) return 0;

	while (isdigit(*s)) s++;

	if (*s == c || '\0' == *s)
		return 1;
	else if ('.' == *s)
		s++;
	else
		return 0;

	while (isdigit(*s)) s++;
	while (isgarbage(*s)) s++;    

	if (*s == c || '\0' == *s)
		return 1;
	else
		return 0;
}

/*
 * returns 1 of s is an integer up until the character c or '\0' is reached,
 * otherwhise returns 0
 */
inline unsigned isintc(const char s[restrict static 1], int c)
{
	while (isgarbage(*s)) s++;    

	if('+' == *s || '-' == *s) s++;

	if (!isdigit(*s)) return 0;

	while (isdigit(*s)) s++;
	while (isgarbage(*s)) s++;    

	if (*s == c || '\0' == *s)
		return 1;
	else
		return 0;
}

#endif
