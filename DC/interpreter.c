#include "interpreter.h"

static char *strclean(const char *);
static unsigned isgarbage(int c);

static char *strclean(const char *s) {
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

    upper(t); // make case insensitive
    return trim(t);
}

double interpret(CaseTree *ct) {

}

CaseTree *buildTree(const char *s) {
    char *t = strclean(s);
    char *c, *sc, *es, *x;
    CaseTree *ct = (CaseTree *)malloc(sizeof(CaseTree));

    if ((t = strstr(t, SC)) == NULL) {
	printf("Error in %s: Something went wrong with error checking in UI, there are no cases "
		"in the assumption, so user should have chosen fixed amount for assumption.\n"
		, __func__); 	
	exit(1);
    }

    /* at the root there is a rule (f.e. age, cat, reg, ...) */
    t += strlen(SC);
    for (int i = 0; *t != ' ' && i < RULESIZE; i++)
	ct->rule[i] = *t++;
    ct->rule[RULESIZE] = '\0';

    for (CaseTree *pct = ct; pct != NULL; pct = pct->next) {
	t += strlen(C);
	pct->cond = t;

	x = strstr(t, X);
	sc = strstr(t, SC);

	if (sc == NULL)
	    t = x;
	else 
	    t = (x < sc ? x : sc);
	*(t - 1) = '\0';

	if (strncmp(t, SC, strlen(SC)) == 0) {
	    char *prevt = t++;

	    /* select cases can be nested, we need to find the final end select of this tree */
	    int nests = 1;
	    sc = strstr(t, SC);
	    es = strstr(t, ES);
	    while (sc != NULL && sc < es) {
		nests++;
		sc++;
		sc = strstr(sc, SC);
	    }
	    while (nests--) {
		t++;
		t = strstr(t, ES);
	    }

	    t += strlen(ES);
	    *t++ = '\0';
	    pct->child = buildTree(prevt);
	}
	else
	    pct->child = NULL;

	pct->expr = t;

	c = strstr(t, C);
	es = strstr(t, ES);

	if (c == NULL || es < c) {
	    t = es;
	    pct->next = NULL;
	}
	else {
	    t = c;
	    pct->next = (CaseTree *)malloc(sizeof(CaseTree));
	    strcpy(pct->next->rule, pct->rule);
	}
	*(t - 1) = '\0';
    }

    return ct;
}

void printTree(CaseTree *ct) {
    printf("%s\n", ct->rule);

    while (ct != NULL) {
	printf("%s\n", ct->cond);
	if (ct->child == NULL)
	    printf("%s\n", ct->expr);
	else
	    printTree(ct->child);
	ct = ct->next;
    }
}

static unsigned isgarbage(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
