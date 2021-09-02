#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <dirent.h>
#include "libraryheader.h"
#include "errorexit.h"

/*
 * inline functions
 */
int isgarbage(int c);
char *trim(char *);
void upper(char *restrict);
int isfloat(const char *restrict);
int isint(const char *restrict);

// replace all occurences of string oldW with newW in s
char *replace(const char s[static 1],
		const char oldW[static 1], const char newW[static 1])
{
	char *result = 0; 
	unsigned i, cnt = 0; 
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
	result = jalloc(i + cnt * (newWlen - oldWlen) + 1, sizeof(*result));

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
int DIRexists(const char dname[static restrict 1])
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

char *strinside(const char s[static 1], const char begin[static restrict 1],
		const char end[static restrict 1])
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

double sum(size_t length, double a[restrict length])
{
	double value = 0;
	for (size_t i = 0; i < length; i++) value += *a++;

	return value;
}

/*
 * wrapper for calloc that dies if calloc returns NULL
 */
void *jalloc(size_t n, size_t size)
{
	void *p = calloc(n, size);
	if (0 == p) die("calloc returned NULL");
	
	return p;
}
