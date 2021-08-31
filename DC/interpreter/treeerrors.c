/* 
 * This file defines the error functions for the building of the Select Case
 * tree.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "libraryheader.h"
#include "errorexit.h"
#include "treeerrors.h"

/* 
 * Set to the error found (if any) while building the tree.
 */
static TreeError terrno = NOERR;

/* 
 * Array with error messages for tree errors
 */
const char *const strterrors[] = 
{
	[NOERR] = "No errors found in tree", 
	[SCERR] = "Select Case without End Select", 
	[ESERR] = "End Select without Select Case", 
	[XERR] = "No expression of the form 'x = *'",
	[CERR] = "Case without Select Case",
	[NOCERR] = "Select Case without Case",
	[UNKRULEERR] = "Select Case has unknown rule",
	[NORULEERR] = "Select Case has no rule", 
	[QUOTERR] = "String condition without quotes \"\"", 
	[SEPERR] = "Conditions not separated by ','", 
	[CONDERR] = "Invalid condition",
	[TOERR] = "Incorrect use of \"To\" operator",
	[ISERR] = "Incorrect use of \"Is\" operator",
	[ELSERR] = "Incorrect use of \"Else\" operator"
};

void setterrno(TreeError te) { terrno = te; }

TreeError getterrno(void) { return terrno; }

const char *strterror(TreeError te) { return strterrors[te]; }

/*
 * Performs initial checks that the tree has the correct structure, i.e.
 * Select Case ...
 * 	Case ...
 * 		x = ...
 * End Select
 */

int isvalidTree(const char *t)
{
	char *c, *sc, *es, *x;
	c = strstr(t, C);
	sc = strstr(t, SC);
	es = strstr(t, ES);
	x = strstr(t, X);

	if (0 == x) {
		setterrno(XERR);	
		return 0;
	}
	/* tree of the form 'x = *' is valid */
	if (0 == c && 0 == sc && 0 == es) {
		return 1;
	} else if (0 == sc && 0 != es) {
		setterrno(ESERR);
		return 0;
	} else if (0 == sc && 0 != c) {
		setterrno(CERR);
		return 0;
	} else if (0 == c) {
		setterrno(NOCERR);
		return 0;
	} else if (0 == es) {
		setterrno(SCERR);
		return 0;
	} else if (0 != c && 0 != sc && 0 != es) {
		if (c < sc) {
			setterrno(CERR);
			return 0;
		} else if (es < c) {
			setterrno(NOCERR);
			return 0;
		} else if (es < sc) {
			setterrno(ESERR);
			return 0;
		} else if (sc < c && c < es) {
			return 1;
		} else {
			printf("[%s] Warning: impossible part of function\n", 
					__func__);
			return 0;
		}
	} else {
		printf("[%s] Warning: impossible part of function\n", 
				__func__);
		return 0;
	}

	return 1;
}

/* 
 * checks the conditions of a case. Returns 0 if an error is found and 1 
 * otherwhise
 */
int isvalidcond(CaseTree *ct)
{
	/* 
	 * n: number of conditions separated by ',' 
	 * (there is atleast 1 condition)
	 */
	int n = 1;
	const char *s = ct->cond;
	char t[strlen(s) + 1];
	char *pt = t;
	char *to = strstr(s, "TO");

	snprintf(t, sizeof(t), "%s", s);
	Cmpfunc *cf = ruleset[ct->rule_index].cf;

	while (*pt)
		if (',' == *pt++) n++;

	pt = strtok(t, ",");

	/*
	 * check whether the case is of the correct form when comparing numbers
	 */
	if (cmpnum == cf) {
		while (n--) {
			while (isgarbage(*pt)) pt++; 
			/*
			 * Case Is ...
			 */
			if ('I' == *pt && 'S' == *(pt + 1)) {
				pt += 2;
				while (isgarbage(*pt)) pt++; 

				if ('<' == *pt || '>' == *pt) {
					pt++;
					if ('=' == *pt) pt++;

					while (isgarbage(*pt)) pt++;

					if (!isdigit(*pt)) {
						setterrno(ISERR);
						return 0;
					}

					while (isdigit(*pt)) pt++;

					if ('.' == *pt) {
						pt++;

						if (!isdigit(*pt)) {
							setterrno(ISERR);
							return 0;
						}

						while (isdigit(*pt)) pt++;

						while (isgarbage(*pt)) pt++;

						if ('\0' != *pt) {
							setterrno(ISERR);
							return 0;
						}
					}
				} else if (isdigit(*pt)) {
					while (isdigit(*pt)) pt++;

					while (isgarbage(*pt)) pt++;

					if ('\0' != *pt) {
						setterrno(ISERR);
						return 0;
					}
				} else {
					setterrno(ISERR);
					return 0;
				}
			} else if (0 != to) { /* Case [0-9]+ To [0-9]+ */
				if (!isdigit(*pt)) {
					setterrno(TOERR);
					return 0;
				}

				while (isdigit(*pt)) pt++;

				if ('.' == *pt) {
					pt++;
					while (isdigit(*pt))
						pt++;
				}

				while (isgarbage(*pt)) pt++;

				if ('T' == *pt && 'O' == *(pt + 1)) {
					pt += 2;

					while (isgarbage(*pt)) pt++;

					while (isdigit(*pt)) pt++;

					if ('.' == *pt) {
						pt++;
						while (isdigit(*pt)) pt++;
					}

					while (isgarbage(*pt)) pt++;

					if ('\0' != *pt) {
						setterrno(TOERR);
						return 0;
					}
				} else {
					setterrno(TOERR);
					return 0;
				}
			} else if (0 == strncmp(pt, E, strlen(E))) {
				pt += strlen(E);

				while (isgarbage(*pt)) pt++;

				if ('\0' != *pt) {
					setterrno(ELSERR);
					return 0; 
				}
			} else if (0 == to) {
				while (isdigit(*pt)) pt++;

				while (isgarbage(*pt)) pt++;

				if ('\0' != *pt) {
					setterrno(CONDERR);
					return 0;
				}
			} else errExit("[%s] impossible condition reached\n", 
					__func__);

			pt = strtok(0, ",");
		}
	} else if (cmpstr == cf) {
		while (*s) {
			while (isgarbage(*s)) s++; 

			/* all string conditions should be inside quotes */
			if ('"' == *s++) {
				while ('\0' != *s && '"' != *s) s++;
				if ('\0' == *s) {
					setterrno(QUOTERR); 
					return 0;
				} else if ('"' == *s) {
					s++;
					while (isgarbage(*s)) s++;

					if (',' == *s) {
						; /* OK, continue */
					} else if ('\0' == *s) {
						break; /* OK, continue */
					} else if ('"' == *s) {
						setterrno(SEPERR);			
						return 0;
					} else {
						setterrno(QUOTERR); 
						return 0;
					}
				}
				else errExit("[%s] condition while loop " 
						"impossible", __func__);
			}
			else if (0 == strncmp(ct->cond, E, strlen(E))) {
				return 1;
			} else {
				setterrno(QUOTERR);
				return 0;
			}

			s++;
		}
	} else errExit("[%s] condition while loop impossible (cf unknown)", 
			__func__);

	return 1;
}
