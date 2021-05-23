#include "interpreter.h"

static char *strclean(const char *);
static unsigned isgarbage(int c);
static unsigned cmpnum(CaseTree *ct, double);
static unsigned cmpstr(CaseTree *ct, const char *);

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

static unsigned cmpnum(CaseTree *ct, double f) {
    char tmp[strlen(ct->cond)+1];
    strcpy(tmp, ct->cond);
    char *pt = tmp;

    int n = 1; // number of conditions separated by ',' (there is atleast 1 condition)
    while (*pt)
	if (*pt++ == ',')
	    n++;

    char *cond = strtok(tmp, ",");     
    while (n--) {

	/* in case there is a "TO" operator */
	char *to;
	if ((to = strstr(cond, "TO")) != NULL) {

	    while (!isdigit(*cond))
		cond++; 
	    while (!isdigit(*to))
		to++;

	    if (f >= atof(cond) && f <= atof(to))
		return 1;
	}

	/* in case of "<" or "<=" */
	else if ((to = strchr(cond, '<')) != NULL) {

	    while (!isdigit(*cond))
		cond++;

	    if (*++to == '=') {
		if (f <= atof(cond))
		    return 1;
	    }
	    else {
		if (f < atof(cond))
		    return 1;
	    }
		
	}

	/* in case of ">" or ">=" */
	else if ((to = strchr(cond, '>')) != NULL) {

	    while (!isdigit(*cond))
		cond++;

	    if (*++to == '=') {
		if (f >= atof(cond))
		    return 1;
	    }
	    else {
		if (f > atof(cond))
		    return 1;
	    }
		
	}

	/* if there is an else */
	else if ((to = strstr(cond, "ELSE")) != NULL) {
	    return 1;
	}

	/* in case it's just a fixed amount */
	else {

	    while (!isdigit(*cond))
		cond++;
	    
	    if (f == atof(cond))
		return 1;
	}

	cond = strtok(NULL, ",");
    }

    return 0;
}

static unsigned cmpstr(CaseTree *ct, const char *s) {
    char tmp[strlen(ct->cond)+1];
    strcpy(tmp, ct->cond);
    char *pt = tmp;

    int n = 1; // number of conditions separated by ',' (there is atleast 1 condition)
    while (*pt)
	if (*pt++ == ',')
	    n++;

    char *cond = strtok(tmp, ",");     
    while (n--) {
	cond = strinside(cond, "\"", "\"");
	if (strcmp(cond, s) == 0) {
	    free(cond);
	    return 1;
	}
	free(cond);
	cond = strtok(NULL, ",");
    }

    return 0;
}

double interpret(CaseTree *ct, double age, char *reg, char *cat) {
    double x = 0.0;
    for (CaseTree *pct = ct; pct != NULL; ) {

	while (!isdigit(*pct->expr))
	    pct->expr++;
	x = atof(pct->expr);

	if (strcmp(pct->rule, "AGE") == 0 && cmpnum(pct, age)) {
	    if ((pct = pct->child) == NULL)
		return x;
	    else 
		continue;
	}
	else if (strcmp(pct->rule, "REG") == 0 && cmpstr(pct, reg)) {
	    if ((pct = pct->child) == NULL)
		return x;
	    else 
		continue;
	}
	else if (strcmp(pct->rule, "CAT") == 0 && cmpstr(pct, cat)) {
	    if ((pct = pct->child) == NULL)
		return x;
	    else 
		continue;
	}

	pct = pct->next;
    }

    printf("Warning in %s: no case was found in tree:\n", __func__);
    printTree(ct);
    printf("The assumption will be equal to zero in this case.\n");
    return x;
}

static unsigned isgarbage(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ':';
}
