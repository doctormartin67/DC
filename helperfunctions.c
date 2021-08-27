#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include "libraryheader.h"
#include "errorexit.h"

char *trim(char *s)
{
	char *t = s;
	while (isgarbage(*s)) s++;
	t = s;
	while (*s) s++;
	s--;
	while (isgarbage(*s)) *s-- = '\0';

	return t;
}

void upper(char *s)
{
	while(*s) {
		*s = toupper(*s);
		s++;
	}
}

int isfloat(const char *s)
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

int isint(const char *s)
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

int isgarbage(int c) 
{
	const char *s = GARBAGE;

	while (*s)
		if (c == *s++)
			return 1;
	return 0;
}

// replace all occurences of string oldW with newW in s
char *replace(const char *s, const char *oldW, 	const char *newW)
{
	char *result = 0; 
	int i, cnt = 0; 
	size_t newWlen = strlen(newW); 
	size_t oldWlen = strlen(oldW); 

	// Counting the number of times old word 
	// occur in the string 
	for (i = 0; s[i] != '\0'; i++) {
		if (strstr(&s[i], oldW) == &s[i]) {
			cnt++; 

			// Jumping to index after the old word. 
			i += oldWlen - 1; 
		} 
	} 

	// Making new string of enough length 
	result = jalloc(1, i + cnt * (newWlen - oldWlen) + 1); 

	i = 0; 
	while (*s) {
		// compare the substring with the result 
		if (strstr(s, oldW) == s) {
			strcpy(&result[i], newW); 
			i += newWlen; 
			s += oldWlen; 
		} else
			result[i++] = *s++; 
	} 

	result[i] = '\0'; 
	return result; 
}

// Check if DIR exists
int DIRexists(const char *dname)
{
	DIR* dir = opendir(dname);
	if (dir) {
		/* Directory FILEexists. */
		return 1;
		closedir(dir);
	} else if (ENOENT == errno) {
		/* Directory does not exist. */
		return 0;
	} else {
		/* opendir() failed for some other reason. */
		return -1;
	}
}

/*
 * finds the first occurence of 'begin' and 'end' inside 's' and returns a
 * pointer to the string that lies between 'begin' and 'end'. Returns NULL if
 * neither were found inside 's'.
 */

char *strinside(const char *s, const char *begin, const char *end)
{
	char *pb = 0; // pointer to begin in s
	char *pe = 0; // pointer to end in s
	if (0 == (pb = strstr(s, begin)))
		return 0;
	/* 
	 * pe should start looking for end starting at begin (+1 just in case begin
	 * and end are the same string, otherwise the strstr function would find 
	 * the same string and pb would be equal to pe)
	 */
	pe = pb + 1;
	if (0 == (pe = strstr(pe, end)))
		return 0;

	/* move pointer to start of the value we want */
	pb += strlen(begin);

	return pb;
}

// MAKE SURE YOU INPUT DOUBLE AS ARGUMENTS OR YOU WILL HAVE UNDEFINED BEHAVIOUR!
double min(int argc, ...)
{
	va_list valist;
	double value;
	double temp;

	/* initialize valist for num number of arguments */
	va_start(valist, argc);

	/* access all the arguments assigned to valist */
	value = va_arg(valist, double);
	for (int i = 0; i < argc - 1; i++) {
		temp = va_arg(valist, double);
		value = (value < temp ? value : temp);
	}

	/* clean memory reserved for valist */
	va_end(valist);

	return value;
}

// MAKE SURE YOU INPUT DOUBLE AS ARGUMENTS OR YOU WILL HAVE UNDEFINED BEHAVIOUR!
double max(int argc, ...)
{
	va_list valist;
	double value;
	double temp;

	/* initialize valist for num number of arguments */
	va_start(valist, argc);

	/* access all the arguments assigned to valist */
	value = va_arg(valist, double);
	for (int i = 0; i < argc - 1; i++) {
		temp = va_arg(valist, double);
		value = (value > temp ? value : temp);
	}

	/* clean memory reserved for valist */
	va_end(valist);

	return value;
}

double sum(double a[], int length)
{
	double value = 0;
	for (int i = 0; i < length; i++) value += *a++;

	return value;
}

/*
 * wrapper for calloc that errExit's if calloc returns NULL
 */
void *jalloc(size_t n, size_t size)
{
	void *p = calloc(n, size);
	if (0 == p) errExit("calloc returned NULL\n");
	
	return p;
}
