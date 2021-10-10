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
#include "calculator.h"

/* 
 * Set to the error found (if any) while building the tree.
 */
static TreeError terrno = NOERR;

/* 
 * Array with error messages for tree errors
 */
const char *const strterrors[TERR_AMOUNT] = 
{
	[NOERR] = "No errors found in tree", 
	[SCERR] = "Select Case without End Select", 
	[ESERR] = "End Select without Select Case", 
	[XERR] = "Expression not of the form 'xyzvalue = *'",
	[CERR] = "Case without Select Case",
	[NOCERR] = "Select Case without Case",
	[UNKRULEERR] = "Select Case has unknown rule",
	[NORULEERR] = "Select Case has no rule", 
	[QUOTERR] = "String condition without quotes \"\"", 
	[SEPERR] = "Conditions not separated by ','", 
	[CONDERR] = "Invalid condition",
	[TOERR] = "Incorrect use of \"To\" operator",
	[ISERR] = "Incorrect use of \"Is\" operator",
	[ELSERR] = "Incorrect use of \"Else\" operator",
	[NULLERR] = "Tree string is NULL",
	[COMMAERR] = "Invalid use of ',' separator in expression",
	[TWOOPERR] = "'*' or '/' followed by another '*' or '/'",
	[OPERANDERR] = "No operator in between two operands",
	[INVOPERR] = "Invalid operator",
	[FUNCBRERR] = "Open bracket '(' missing after function name",
	[NOEXPRBRERR] = "Missing expression within brackets",
	[INCBRERR] = "Inconsistent usage of brackets '(' ')'",
	[NUMERR] = "One or more string variables used as double"
};

void setterrno(TreeError te) { 
	if (te < NOERR || te >= TERR_AMOUNT)
		die("%d is not a valid TreeError", te);
	terrno = te;
}

TreeError getterrno(void) { return terrno; }

const char *strterror(TreeError te) { return strterrors[te]; }

static unsigned isvalid_CaseIs(const char s[static 1]);
static unsigned isvalid_Case_a_To_b(const char s[static 1]);
static unsigned isvalid_CaseElse(const char s[static 1]);
static unsigned isvalid_CaseNumber(const char s[static 1]);

/*
 * Performs initial checks that the tree has the correct structure, i.e.
 * Select Case ...
 * 	Case ...
 * 		xyzvalue = ...
 * End Select
 */
unsigned isvalidTree(const char *t)
{
	if (0 == t) {
		setterrno(NULLERR);	
		return 0;
	}

	char *c, *sc, *es, *x;
	c = strstr(t, C);
	sc = strstr(t, SC);
	es = strstr(t, ES);
	x = strstr(t, X);

	if (0 == x) {
		setterrno(XERR);	
		return 0;
	}

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
 * checks the conditions of a case. if compare function is a number, strtok
 * is used to split conditions and there is atleast 1 condition 'n'.
 * Returns 0 if an error is found and 1 otherwhise
 */
unsigned isvalidBranch(const struct casetree ct[static 1])
{
	unsigned n = 1;
	const char *s = ct->cond;
	char t[strlen(s) + 1];
	const char *pt = t;
	const char *to = strstr(s, "TO");

	snprintf(t, sizeof(t), "%s", s);
	Cmpfunc *cf = 0;
	if (var_set[ct->rule_index].is_number)
		cf = cmpnum;
	else
		cf = cmpstr;

	/*
	 * check whether the case is of the correct form when comparing numbers
	 */
	if (cmpnum == cf) {
		while (*pt)
			if (',' == *pt++) n++;

		pt = strtok(t, ",");

		while (n--) {
			while (isgarbage(*pt)) pt++; 
			if ('I' == *pt && 'S' == *(pt + 1)) {
				if (!isvalid_CaseIs(pt))
					return 0;
			} else if (0 != to) {
				if (!isvalid_Case_a_To_b(pt))
					return 0;
			} else if (0 == strncmp(pt, E, strlen(E))) {
				if (!isvalid_CaseElse(pt))
					return 0;
			} else if (0 == to) {
				if (!isvalid_CaseNumber(pt))
					return 0;
			} else die("impossible condition reached");

			pt = strtok(0, ",");
		}
	} else if (cmpstr == cf) {
		while (*s) {
			while (isgarbage(*s)) s++; 

			if ('"' == *s++) {
				while ('\0' != *s && '"' != *s) s++;

				if ('\0' == *s) {
					setterrno(QUOTERR); 
					return 0;
				} else if ('"' == *s) {
					s++;
					while (isgarbage(*s)) s++;

					if (',' == *s) {
						s++;
					} else if ('\0' == *s) {
						break;
					} else if ('"' == *s) {
						setterrno(SEPERR);			
						return 0;
					} else {
						setterrno(QUOTERR); 
						return 0;
					}
				} else
					die("condition while loop "
							"impossible");
			}
			else if (0 == strncmp(ct->cond, E, strlen(E))) {
				return 1;
			} else {
				setterrno(QUOTERR);
				return 0;
			}
		}
	} else die("condition while loop impossible (cf unknown)");

	return 1;
}

/*
 * checks if the leaf is a valid expression that begins with 'XYZVALUE='
 * followed by a valid calculator expression. It may also begin with
 * Select Case, in which case the tree has branches and we will check the
 * leaves of these branches at a later stage
 */
unsigned isvalidLeaf(const char s[static 1])
{
	if (0 == strncmp(s, SC, strlen(SC)))
		return 1;

	if (0 != strncmp(s, X, strlen(X))) {
		setterrno(XERR);
		return 0;
	}

	s += strlen(X);
	
	while (isgarbage(*s)) s++;

	if ('=' != *s++) {
		setterrno(XERR);
		return 0;
	}

	if (!valid_brackets(s) || !valid_separators(s))
		return 0;
	
	if (NOERR == getterrno()) {
		eval_expr(s);
		if (NOERR != getterrno())
			return 0;
	}

	return 1;
}

/*
 * checks whether Case Is is valid, f.e.
 * Case is < 40
 */
static unsigned isvalid_CaseIs(const char s[static 1])
{
	s += 2;
	while (isgarbage(*s)) s++; 

	if ('<' == *s || '>' == *s) s++;
	if ('=' == *s) s++;

	if (!isfloat(s)) {
		setterrno(ISERR);
		return 0;
	} else	
		return 1;
}

/*
 * checks whether Case a To b is valid, f.e.
 * Case 1 To 10
 */
static unsigned isvalid_Case_a_To_b(const char s[static 1])
{
	if (!isdigit(*s)) {
		setterrno(TOERR);
		return 0;
	}

	while (isdigit(*s)) s++;

	if ('.' == *s) {
		s++;
		while (isdigit(*s)) s++;
	}

	while (isgarbage(*s)) s++;

	if ('T' == *s && 'O' == *(s + 1)) {
		s += 2;

		if (!isfloat(s)) {
			setterrno(TOERR);
			return 0;
		}
	} else {
		setterrno(TOERR);
		return 0;
	}

	return 1;
}

/*
 * checks whether Case Else is valid
 */
static unsigned isvalid_CaseElse(const char s[static 1])
{
	s += strlen(E);

	while (isgarbage(*s)) s++;

	if ('\0' != *s) {
		setterrno(ELSERR);
		return 0; 
	}

	return 1;
}

/*
 * checks whether Case number is valid, f.e.
 * Case 55
 */
static unsigned isvalid_CaseNumber(const char s[static 1])
{
	if (!isfloat(s)) {
		setterrno(CONDERR);
		return 0;
	}

	return 1;
}
