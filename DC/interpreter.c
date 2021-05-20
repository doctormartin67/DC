#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libraryheader.h"

char *strclean(const char *);
unsigned isgarbage(int c);
static char *numcase(double d, const char *s, const char *t);

char *strclean(const char *s) {
    char *t = calloc(strlen(s) + 1, sizeof(char));
    char *pt = t;
    
    while (*s) {
	if (!isgarbage(*s))
	    *pt++ = *s++;
	else {
	    *pt++ = ' ';
	    while (isgarbage(*s))
		s++;
	}
    }
    *pt = '\0';

    return trim(t);
}

double interpretass(double age, char *cat, char *reg, const char *s) {
    double x = 0;  
    const char *orig = s;

    if (strstr(s, "SELECT CASE") == NULL) {
	s = strchr(s, 'X');
	while (!isdigit(*s) && *s != '\0')
	    s++;
	if (*s == '\0') {
	    printf("Error in %s: The following string was given:\n"
		    "-------\n"
		    "%s\n"
		    "-------\n"
		    "This string shouldn't have gotten through the checks\n", __func__, orig);
	    exit(1);
	}
	return atof(s);
    }
    else {
	s = strstr(s, "CASE");
	s += 4;
	while (isgarbage(*s))
	    s++;
	if (strncmp(s, "AGE", 3) == 0) {

	}
	else if (strncmp(s, "REG", 3) == 0) {

	}
	else if(strncmp(s, "CAT", 3) == 0) {

	}
	else {
	    printf("Error in %s: The following string was given:\n"
		    "-------\n"
		    "%s\n"
		    "-------\n"
		    "The variable with the first 3 characters: \'%s\' is not "
		    "a known variable. This string should not have passed the checks in the UI.\n"
		    , __func__, orig, s);
	    exit(1);
	}
    }
}

/* searches case between s and t */
static char *numcase(double d, const char *s, const char *t) {
    const char *orig = s;
    const char *snext = s;
    while ((s = strstr(s, "CASE")) && s < t) {
	s += strlen("CASE");	
	while (isgarbage(*s))
	    s++;

    }

    if (s == NULL) {
	printf("Error in %s: The string\n"
		"------\n"
		"%s\n"
		"------\n"
		"is part of a select case but without a \'case\' to go along with it\n", 
		__func__, s);
	exit(1);
    }
    else {
	printf("Warning in %s: none of the cases in "
		"------\n"
		"%s\n"
		"------\n"
		"correspond to the amount %f. Return NULL.\n", __func__, orig, d);
	return NULL;
    }
}

unsigned isgarbage(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
