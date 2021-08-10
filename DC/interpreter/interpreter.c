/* This is an interpreter than will interpret a text buffer that will be input
   as VBA Select Case syntax by the user. To understand more about how that
   works it's best to read up on the syntax of Select Case. CaseTree is a 
   linked list that will point to the various cases input by the user. 
   The tree consists of all the Select Cases with the root of the tree being 
   determined by the rule and each case points to the next one. If there is 
   a select case within another select case then 'child' will point to the
   first case of the subtree. */

#include "treeerrors.h"

/* ruleset is an array which consists of the variables that the user
   can use to Select Case over. They can be seen as the predetermined
   variables instead of the user having to define them themselves. */
/* data (NULL) will be set each time interpret is called because it can
   be different each time */
Rule ruleset[] = 
{
    {"AGE", cmpnum, NULL}, {"REG", cmpstr, NULL}, {"CAT", cmpstr, NULL}
};

/* Before the tree is built, an initial clean is run so that the string to
   build the tree consists of words separated by 1 space. The string is also
   set to uppercase to make the interpreter case insensitive. */
char *strclean(const char *s)
{
    char *t = calloc(strlen(s) + 1, sizeof(char));
    if (NULL == t) errExit("[%s] calloc returned NULL\n", __func__);

    char *pt = t;

    while (*s)
    {
	if (!isgarbage(*s))
	    *pt++ = *s++;
	else
	{
	    *pt++ = ' ';
	    while ('\0' != *s && isgarbage(*s))
		s++;
	}
    }
    *pt = '\0';

    upper(t); // make case insensitive

    /* END SELECT needs to be replaced with something else, because I need a 
       unique identifier to find the beginning and end of a select case. If 
       there is a case after end select, then strstr(s, "SELECT CASE") would 
       find this when in fact this isn't the beginning of a select case, it's 
       the end of the previous one. I chose "END SELEC67" */
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
CaseTree *buildTree(const char *s)
{
    if (!isvalidTree(s)) return NULL;
    char *t = strdup(s);
    char *const pt = t; /* used to free t */
    char *c, *sc, *es, *x;
    c = sc = es = x = NULL;
    char *rulename = NULL;

    CaseTree *ct = (CaseTree *)malloc(sizeof(CaseTree));
    if (ct == NULL) errExit("[%s] malloc returned NULL\n", __func__);
    ct->rule_index = -1; /* no rule yet */
    ct->cond = ct->expr = NULL;
    ct->next = ct->child = NULL;

    if (NULL == (t = strstr(pt, SC)))
    { 
	ct->expr = strstr(pt, X);
	return ct;
    }

    /* at the root there is a rule (f.e. age, cat, reg, ...) */
    t += strlen(SC); 
    t++; /* space ' ' at the end of SC */
    rulename = t;
    while ('\0' != *t && ' ' != *t)
	t++;
    *t++ = '\0';

    /* return NULL if the next word is 'case', meaning there was no rule */
    if (0 == strncmp(rulename, C, strlen(C)))
    {
	free(pt);
	freeTree(ct);
	setterrno(NORULEERR);
	return NULL;
    }

    ct->rule_index = setRule(rulename);

    /* return NULL if the next word was not a known rule */
    if (-1 == ct->rule_index) 
    {
	free(pt);
	freeTree(ct);
	setterrno(UNKRULEERR);
	return NULL;
    }

    for (CaseTree *pct = ct; NULL != pct; pct = pct->next)
    {
	/* return NULL if there is no 'case' right after select case 'rule' */
	if (0 != strncmp(t, C, strlen(C)))
	{
	    free(pt);
	    freeTree(ct);
	    setterrno(NOCERR);
	    return NULL;
	}

	t += strlen(C);
	t++; /* space ' ' at the end of C */
	pct->cond = t;

	x = strstr(t, X);
	sc = strstr(t, SC);

	if (NULL == sc)
	    t = x;
	else 
	    t = (x < sc ? x : sc);

	*(t - 1) = '\0';
	if (!isvalidcond(pct)) /* terrno set by isvalidcond */
	{
	    free(pt);
	    free(ct);
	    return NULL;
	}

	pct->expr = t;

	if (0 == strncmp(t, SC, strlen(SC)))
	{
	    char *prevt = t++;

	    /* select cases can be nested, we need to find the final end select 
	       of this tree */
	    int nests = 1;

	    while (nests)
	    {
		sc = strstr(t, SC);
		es = strstr(t, ES);

		if (NULL == sc || sc > es)
		{
		    t = es;
		    nests--;
		}
		else
		{
		    t = sc;
		    nests++;
		}
		t++;
	    }

	    /* when nests hits zero, t should be at end select 
	       (the +1 is because we add 1 to t in the while loop) */
	    assert(t == es + 1);

	    t += strlen(ES) - 1;
	    *t++ = '\0';
	    pct->child = buildTree(prevt);
	}
	else
	    pct->child = NULL;

	c = strstr(t, C);
	es = strstr(t, ES);

	if (NULL == es)
	{
	    free(pt);
	    freeTree(ct);
	    setterrno(SCERR);
	    return NULL;
	}

	if (NULL == c || es < c)
	{
	    t = es;
	    pct->next = NULL;
	}
	else
	{
	    t = c;
	    if (NULL == (pct->next = malloc(sizeof(CaseTree))))
		errExit("[%s] malloc return NULL\n", __func__);

	    pct->next->rule_index = pct->rule_index;
	}
	*(t - 1) = '\0';
    }

    return ct;
}

/* Returns the index of the ruleset array for the given name in the tree. If
   no rule is found then -1 is returned. */
int setRule(const char *name)
{
    int index = -1; /* this will be set to the index of the rule name, 
		       if it remains -1 then an error occured */
    int len = sizeof(ruleset)/sizeof(ruleset[0]);
    size_t n = strlen(name);
    size_t rn; /* ruleset name size */

    /* find rule */
    for (int i = 0; i < len; i++)
    {
	rn = strlen(ruleset[i].name);
	if (n != rn)
	    continue;
	else if (0 == strncmp(name, ruleset[i].name, n)) 
	    index = i;
    }

    return index;
}

void printTree(CaseTree *ct)
{
    static int cnt = 0; /* count the tabs to be used */
    char tabs[32] = "";
    char temp[32] = ""; /* used for end of tree (end select) */

    for (int i = 0; i < cnt; i++)
	strcat(tabs, "\t");

    if (-1 == ct->rule_index)
    {
	printf("%s\n", ct->expr);
	return;
    }
    else
	printf("%sselect case %s\n", tabs, ruleset[ct->rule_index].name);

    strcpy(temp, tabs); /* remember tabs used here because it will be the same 
			   at end of tree */

    cnt++; /* when cases start we are one level removed from top */
    while (NULL != ct)
    {
	strcpy(tabs, "");
	for (int i = 0; i < cnt; i++)
	    strcat(tabs, "\t");
	printf("%scase %s\n", tabs, ct->cond);

	cnt++; /* either a child or new tree is created, both need another 
		  tab */
	if (NULL == ct->child)
	{
	    strcpy(tabs, "");
	    for (int i = 0; i < cnt; i++)
		strcat(tabs, "\t");
	    printf("%s%s\n", tabs, ct->expr);
	}
	else
	    printTree(ct->child);

	cnt--; /* we added tab for child, subtract one again for next case */
	ct = ct->next;
    }
    cnt--; /* end select is one tab before the cases */
    printf("%send select\n", temp);
}

void freeTree(CaseTree *ct)
{
    if (NULL != ct)
    {
	freeTree(ct->child);	
	freeTree(ct->next);	
	free(ct);
    }
}

int cmpnum(CaseTree *ct, const void *pf)
{
    double f = *((double *)pf);
    char tmp[strlen(ct->cond) + 1];
    snprintf(tmp, sizeof(tmp), "%s", ct->cond);
    char *cond = tmp;

    int n = 1; /* number of conditions separated by ',' 
		  (there is atleast 1 condition) */
    while (*cond)
	if (',' == *cond++)
	    n++;

    cond = strtok(tmp, ",");     
    while (n--)
    {
	/* in case there is a "TO" operator */
	char *to;
	if (NULL != (to = strstr(cond, "TO")))
	{
	    while ('\0' != *cond && !isdigit(*cond))
		cond++; 
	    while ('\0' != *to && !isdigit(*to))
		to++;

	    if (f >= atof(cond) && f <= atof(to))
		return 1;
	}

	/* in case of "<" or "<=" */
	else if (NULL != (to = strchr(cond, '<')))
	{
	    while ('\0' != *cond && !isdigit(*cond))
		cond++;

	    if ('=' == *++to) 
	    {
		if (f <= atof(cond))
		    return 1;
	    }
	    else
	    {
		if (f < atof(cond))
		    return 1;
	    }
	}

	/* in case of ">" or ">=" */
	else if (NULL != (to = strchr(cond, '>')))
	{
	    while ('\0' != *cond && !isdigit(*cond))
		cond++;

	    if ('=' == *++to) 
	    {
		if (f >= atof(cond))
		    return 1;
	    }
	    else
	    {
		if (f > atof(cond))
		    return 1;
	    }
	}

	/* if there is an else */
	else if (NULL != (to = strstr(cond, "ELSE")))
	    return 1;

	/* in case it's just a fixed amount */
	else
	{
	    while ('\0' != *cond && !isdigit(*cond))
		cond++;

	    if (f == atof(cond))
		return 1;
	}

	cond = strtok(NULL, ",");
    }
    return 0;
}

int cmpstr(CaseTree *ct, const void *s)
{
    char tmp[strlen(ct->cond)+1];
    strcpy(tmp, ct->cond);
    char *pt = tmp;

    int n = 1; /* number of conditions separated by ',' 
		  (there is atleast 1 condition) */
    while (*pt)
	if (',' == *pt++)
	    n++;

    char *cond = strtok(tmp, ",");     
    while (n--)
    {
	if (NULL != (strstr(cond, "ELSE"))) /* if there is an else */
	    return 1;

	if (NULL == (cond = strinside(cond, "\"", "\"")))
	    errExit("[%s] string in case should be "
		    "defined between quotes \"\"", __func__);

	if (0 == strcmp(cond, (char *)s))
	{
	    free(cond);
	    return 1;
	}
	
	cond = strtok(NULL, ",");
    }
    return 0;
}

double interpret(CaseTree *ct, const void *rule_data[])
{
    double x = 0.0;
    Cmpfunc *cf;
    const void *v;
    for (CaseTree *pct = ct; NULL != pct; )
    {
	if (-1 == pct->rule_index) /* no rule, just an expression */
	{
	    while ('\0' != *pct->expr && !isdigit(*pct->expr))
		pct->expr++;
	    x = atof(pct->expr);
	    return x;
	}

	v = rule_data[pct->rule_index];
	cf = ruleset[pct->rule_index].cf;

	if (cf(pct, v))
	{
	    if (NULL == (pct->child))
	    {
		while ('\0' != *pct->expr && !isdigit(*pct->expr))
		    pct->expr++;
		x = atof(pct->expr);
		return x;
	    }
	    else
	    {
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
