/* This is an interpreter than will interpret a text buffer that will be input
   as VBA Select Case syntax by the user. To understand more about how that
   works it's best to read up on the syntax of Select Case. CaseTree is a 
   linked list that will point to the various cases input by the user. 
   The tree consists of all the Select Cases with the root of the tree being 
   determined by the rule and each case points to the next one. If there is 
   a select case within another select case then 'child' will point to the
   first case of the subtree. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "libraryheader.h"
#include "treeerrors.h"
#include "errorexit.h"

/* maximum amount of tabs in a tree, printTree won't work properly otherwise */
#define MAXTABS 32

/*
 * ruleset is an array which consists of the variables that the user
 * can use to Select Case over. They can be seen as the predetermined
 * variables instead of the user having to define them themselves.
 * data (NULL) will be set each time interpret is called because it can
 * be different each time
 */
Rule ruleset[RULE_AMOUNT] = 
{
	[AGE] = {"AGE", cmpnum, 0}, 
	[REG] = {"REG", cmpstr, 0}, 
	[CAT] = {"CAT", cmpstr, 0}
};

static int specifyTree(const char t[static 1]);
static void deforest(CaseTree *ct, char *t, TreeError te);
static void settabs(unsigned cnt, char s[static 1]);

/*
 * Before the tree is planted, an initial clean is run so that the string to
 * plant the tree consists of words separated by 1 space. The string is also
 * set to uppercase to make the interpreter case insensitive.
 */
char *strclean(const char s[static 1])
{
	char *t = jalloc(strlen(s) + 1, sizeof(char));
	char *pt = t;

	while (*s) {
		if (!isgarbage(*s))
			*pt++ = *s++;
		else {
			*pt++ = ' ';
			while ('\0' != *s && isgarbage(*s))
				s++;
		}
	}
	*pt = '\0';

	upper(t);

	/*
	 * END SELECT needs to be replaced with something else, because I need 
	 * a unique identifier to find the beginning and end of a select case.
	 * If there is a case after end select, then strstr(s, "SELECT CASE") 
	 * would find this when in fact this isn't the beginning of a select 
	 * case, * it's the end of the previous one. I chose "END SELEC67"
	 */ 
	pt = t;
	t = replace(t, "END SELECT", ES);
	free(pt);
	pt = t;
	t = replace(t, "SELECT CASE", SC);
	free(pt);
	return trim(t);
}

/*
 * Build a tree which is a linked list of 'CaseTree' structs. Each case has 
 * has a rule and a pointer to the next case. If there is a nested select 
 * case then the CaseTree also has a pointer to the first case via the child
 * element. 
 * The tree is checked for errors in this function, some errors are checked 
 * directly, others are checked with the functions 'isvalid*'. 
 */
CaseTree *plantTree(const char *s)
{
	if (!isvalidTree(s)) return 0;

	char *t = strdup(s);
	char *c, *sc, *es, *x;
	char *rulename = 0;
	char *const pt = t;
	c = sc = es = x = 0;

	CaseTree *ct = jalloc(1, sizeof(CaseTree));
	*ct = (CaseTree){0};
	ct->rule_index = -1;

	if ((t = strstr(pt, SC)) == 0) { 
		ct->expr = strstr(pt, X);
		if (!isvalidLeaf(ct->expr)) {
			deforest(ct, pt, NOERR);
			return 0;
		}
		return ct;
	}

	t += strlen(SC) + 1;
	rulename = t;
	while ('\0' != *t && ' ' != *t)
		t++;
	*t++ = '\0';

	if (0 == strncmp(rulename, C, strlen(C))) {
		deforest(ct, pt, NORULEERR);
		return 0;
	}

	ct->rule_index = specifyTree(rulename);

	if (-1 == ct->rule_index) {
		deforest(ct, pt, UNKRULEERR);
		return 0;
	}

	for (CaseTree *pct = ct; 0 != pct; pct = pct->next) {
		if (0 != strncmp(t, C, strlen(C))) {
			deforest(ct, pt, NOCERR);
			return 0;
		}

		t += strlen(C) + 1;
		pct->cond = t;

		x = strstr(t, X);
		sc = strstr(t, SC);

		if (0 == sc)
			t = x;
		else 
			t = (x < sc ? x : sc);

		*(t - 1) = '\0';
		if (!isvalidBranch(pct)) {
			deforest(ct, pt, NOERR);
			return 0;
		}

		pct->expr = t;

		if (0 == strncmp(t, SC, strlen(SC))) {
			char *prevt = t++;

			/*
			 * select cases can be nested, we need to find the 
			 * final end select of this tree
			 */
			int nests = 1;

			while (nests) {
				sc = strstr(t, SC);
				es = strstr(t, ES);

				if (0 == sc || sc > es) {
					t = es;
					nests--;
				} else {
					t = sc;
					nests++;
				}
				t++;
			}

			/*
			 * when nests hits zero, t should be at end select 
			 * (the +1 is because we add 1 to t in the while loop)
			 */
			assert(t == es + 1);

			t += strlen(ES) - 1;
			*t++ = '\0';
			pct->child = plantTree(prevt);
		} else {
			if (!isvalidLeaf(pct->expr)) {
				deforest(ct, pt, NOERR);
				return 0;
			}
			pct->child = 0;
		}

		c = strstr(t, C);
		es = strstr(t, ES);

		if (0 == es) {
			deforest(ct, pt, SCERR);
			return 0;
		}

		if (0 == c || es < c) {
			t = es;
			pct->next = 0;
		} else {
			t = c;
			pct->next = jalloc(1, sizeof(CaseTree));
			pct->next->rule_index = pct->rule_index;
		}
		*(t - 1) = '\0';
	}

	return ct;
}

/* 
 * Returns the index of the ruleset array for the given name in the tree, or
 * -1 of no rule was found
 */
 static int specifyTree(const char name[static restrict 1])
{
	int index = -1;
	size_t n = strlen(name);
	size_t rn = 0;

	for (unsigned i = 0; i < RULE_AMOUNT; i++) {
		rn = strlen(ruleset[i].name);
		if (n != rn)
			continue;
		else if (0 == strncmp(name, ruleset[i].name, n)) 
			index = i;
	}

	return index;
}

void printTree(const CaseTree ct[static 1])
{
	static unsigned cnt = 0;
	char tabs[MAXTABS];
	char temp[MAXTABS];

	settabs(cnt, tabs);
	if (-1 == ct->rule_index) {
		printf("%s\n", ct->expr);
		return;
	}
	else
		printf("%sselect case %s\n", 
				tabs, ruleset[ct->rule_index].name);

	snprintf(temp, sizeof(temp), "%s", tabs);

	cnt++;
	while (0 != ct) {
		settabs(cnt, tabs);
		printf("%scase %s\n", tabs, ct->cond);
		cnt++; 

		if (0 == ct->child) {
			settabs(cnt, tabs);
			printf("%s%s\n", tabs, ct->expr);
		} else
			printTree(ct->child);

		cnt--;
		ct = ct->next;
	}
	cnt--; /* end select is one tab before the cases */
	printf("%send select\n", temp);
}

static void settabs(unsigned cnt, char s[static 1])
{
	for (unsigned i = 0; i < cnt && i < MAXTABS - 1; i++)
		*s++ = '\t';
	*s = '\0';
}

void chopTree(CaseTree *ct)
{
	if (0 != ct) {
		chopTree(ct->child);	
		chopTree(ct->next);	
		free(ct);
	}
}

/*
 * if an incorrect tree is planted, then it is deforested and terrno is set if
 * a TreeError other than NOERR is chosen
 */
static void deforest(CaseTree *ct, char *t, TreeError te)
{
	free(t);
	chopTree(ct);
	if (NOERR != te) setterrno(te);
}

unsigned cmpnum(const CaseTree ct[static restrict 1], const void *pf)
{
	unsigned n = 1;
	double f = *((double *)pf);
	char tmp[strlen(ct->cond) + 1];
	char *cond = tmp;

	snprintf(tmp, sizeof(tmp), "%s", ct->cond);

	while (*cond)
		if (',' == *cond++) n++;

	cond = strtok(tmp, ",");     
	while (n--) {
		/* in case there is a "TO" operator */
		char *to;
		if (0 != (to = strstr(cond, "TO"))) {
			while ('\0' != *cond && !isdigit(*cond))
				cond++; 
			while ('\0' != *to && !isdigit(*to))
				to++;

			if (f >= atof(cond) && f <= atof(to))
				return 1;
		} else if (0 != (to = strchr(cond, '<'))) {
			while ('\0' != *cond && !isdigit(*cond))
				cond++;

			if ('=' == *++to) {
				if (f <= atof(cond))
					return 1;
			} else {
				if (f < atof(cond))
					return 1;
			}
		}

		/* in case of ">" or ">=" */
		else if (0 != (to = strchr(cond, '>'))) {
			while ('\0' != *cond && !isdigit(*cond))
				cond++;

			if ('=' == *++to) {
				if (f >= atof(cond))
					return 1;
			} else {
				if (f > atof(cond))
					return 1;
			}
		} else if (0 != (to = strstr(cond, "ELSE")))
			return 1;

		/* in case it's just a fixed amount */
		else {
			while ('\0' != *cond && !isdigit(*cond))
				cond++;

			if (f == atof(cond))
				return 1;
		}

		cond = strtok(0, ",");
	}
	return 0;
}

/*
 * compare function that takes a string as a void pointer and compares it with
 * the condition of the rule. The while loop with break if either the
 * condition has no more quotes '"', or if the string is equal to the string
 * inside one of the quotes.
 * returns 1 if condition matches string in quotes, 0 otherwise
 */
unsigned cmpstr(const CaseTree ct[static restrict 1], const void *s)
{
	if (0 == strncmp(ct->cond, E, strlen(E)))
		return 1;

	const char *ps = 0;
	const char *cond = strinside(ct->cond, "\"", "\"");
	const char *quote = strchr(cond, '"');

	while (1) {
		ps = s;
		while ('\0' != *ps && cond < quote) {
			if (*ps++ != *cond++)
				break;
		}
		if (cond == quote)
			return 1;

		cond = strinside(quote + 1, "\"", "\"");
		if (0 != cond)
			quote = strchr(cond, '"');
		else
			break;
	}
	return 0;
}

double interpret(const CaseTree ct[static 1],
		const void *rule_data[static RULE_AMOUNT])
{
	double x = 0.0;
	Cmpfunc *cf = 0;
	const void *v = 0;
	const char *expr = 0;
	for (const CaseTree *pct = ct; 0 != pct; ) {
		expr = pct->expr;
		if (-1 == pct->rule_index) {
			while ('\0' != *expr && !isdigit(*expr))
				expr++;
			x = atof(expr);
			return x;
		}

		v = rule_data[pct->rule_index];
		cf = ruleset[pct->rule_index].cf;

		if (cf(pct, v)) {
			if (0 == (pct->child)) {
				while ('\0' != *expr && !isdigit(*expr))
					expr++;
				x = atof(expr);
				return x;
			} else {
				pct = pct->child;
				continue;
			}
		}

		pct = pct->next;
	}

	printf("Warning in %s: no case was found in tree:\n", __func__);
	printTree(ct);
	printf("The assumption will be equal to zero in this case.\n");
	return x;
}
