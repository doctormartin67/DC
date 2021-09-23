/* 
 * This is an interpreter than will interpret a text buffer that will be input
 * as VBA Select Case syntax by the user. To understand more about how that
 * works it's best to read up on the syntax of Select Case. struct casetree is
 * a linked list that will point to the various cases input by the user. 
 * The tree consists of all the Select Cases with the root of the tree being 
 * determined by the rule and each case points to the next one. If there is 
 * a select case within another select case then 'child' will point to the
 * first case of the subtree.
 */

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

static struct casetree *growBranch(char **t);
static int specifyTree(const char t[static 1]);
static void deforest(struct casetree *ct, char *t, TreeError te);
static void settabs(unsigned cnt, char s[static 1]);

/*
 * Before the tree is planted, an initial clean is run so that the string to
 * plant the tree consists of words separated by 1 space. The string is also
 * set to uppercase to make the interpreter case insensitive.
 */
char *strclean(const char s[static 1])
{
	char *t = jalloc(strlen(s) + 1, sizeof(*t));
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
 * Build a tree which is a linked list of casetree structs. Each case has 
 * has a rule and a pointer to the next case. If there is a nested select 
 * case then the casetree also has a pointer to the first case via the child
 * element. 
 * The tree is checked for errors in this function, some errors are checked 
 * directly, others are checked with the functions 'isvalid*'. 
 */
struct casetree *plantTree(const char *s)
{
	if (!isvalidTree(s)) return 0;

	char *t = strdup(s);
	char *c, *sc, *es, *x;
	char *rulename = 0;
	char *const pt = t;
	c = sc = es = x = 0;

	struct casetree *ct = jalloc(1, sizeof(*ct));
	*ct = (struct casetree){0};
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

	for (struct casetree *pct = ct; 0 != pct; pct = pct->next) {
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
			pct->child = growBranch(&t);
		} else {
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
			pct->next = jalloc(1, sizeof(*pct->next));
			pct->next->rule_index = pct->rule_index;
		}
		*(t - 1) = '\0';

		if (!isvalidLeaf(pct->expr)) {
			deforest(ct, pt, NOERR);
			return 0;
		}
	}

	return ct;
}

/*
 * When trees are nested we need to grow a child tree within a tree.
 * finds where the current tree ends, taking into account more potential
 * branches and returns the tree
 */
static struct casetree *growBranch(char **t)
{
	struct casetree *ct = 0;
	char *pt = (*t)++;
	char *sc, *es;
	int nests = 1;
	sc = es = 0;

	while (nests) {
		sc = strstr(*t, SC);
		es = strstr(*t, ES);

		if (0 == sc || sc > es) {
			*t = es;
			nests--;
		} else {
			*t = sc;
			nests++;
		}
		(*t)++;
	}

	/*
	 * when nests hits zero, t should be at end select 
	 * (the +1 is because we add 1 to t in the while loop)
	 */
	assert(*t == es + 1);

	(*t) += strlen(ES) - 1;
	*(*t)++ = '\0';
	ct = plantTree(pt);

	return ct;
}

/* 
 * Returns the index of the var_set array for the given name in the tree, or
 * -1 of no rule was found
 */
 static int specifyTree(const char name[restrict static 1])
{
	int index = -1;
	size_t n = strlen(name);
	size_t rn = 0;

	for (unsigned i = 0; i < VAR_AMOUNT; i++) {
		rn = strlen(var_set[i].name);
		if (n != rn)
			continue;
		else if (0 == strncmp(name, var_set[i].name, n)) 
			index = i;
	}

	return index;
}

void printTree(const struct casetree ct[static 1])
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
				tabs, var_set[ct->rule_index].name);

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

void chopTree(struct casetree *ct)
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
static void deforest(struct casetree *ct, char *t, TreeError te)
{
	free(t);
	chopTree(ct);
	if (NOERR != te) setterrno(te);
}

unsigned cmpnum(const struct casetree ct[restrict static 1], const void *pf)
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
unsigned cmpstr(const struct casetree ct[restrict static 1], const void *s)
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

double interpret(const struct casetree ct[static 1],
		const void *const rule_data[const static VAR_AMOUNT])
{
	double x = 0.0;
	Cmpfunc *cf = 0;
	const void *v = 0;
	const char *expr = 0;

	while (0 != ct) {
		expr = ct->expr;
		if (-1 == ct->rule_index) {
			while ('\0' != *expr && '=' != *expr)
				expr++;
			if ('=' == *expr) expr++;
			x = eval_expr(expr);
			return x;
		}

		v = rule_data[ct->rule_index];
		if (var_set[ct->rule_index].is_number)
			cf = cmpnum;
		else
			cf = cmpstr;

		if (cf(ct, v)) {
			if (0 == (ct->child)) {
				while ('\0' != *expr && '=' != *expr)
					expr++;
				if ('=' == *expr) expr++;
				x = eval_expr(expr);
				return x;
			} else {
				ct = ct->child;
				continue;
			}
		} else {
			ct = ct->next;	
		}
	}

	printf("Warning: End of tree reached with no case matched\n");
	return 0.0;
}
