/* This is an interpreter than will interpret a text buffer that will be input
   as VBA Select Case syntax by the user. To understand more about how that
   works it's best to read up on the syntax of Select Case. CaseTree is a 
   linked list that will point to the various cases input by the user. 
   The tree consists of all the Select Cases with the root of the tree being 
   determined by the rule and each case points to the next one. If there is 
   a select case within another select case then 'child' will point to the
   first case of the subtree. */

#include "interpreter.h"

static Cmpfunc cmpnum;
static Cmpfunc cmpstr;

/* data (NULL) will be set each time interpret is called because it can
   be different each time */
Rule ruleset[] = 
{
    {"AGE", cmpnum, NULL}, {"REG", cmpstr, NULL}, {"CAT", cmpstr, NULL}
};

char *strclean(const char *s)
{
    char *t = calloc(strlen(s) + 1, sizeof(char));
    if (t == NULL) errExit("[%s] calloc returned NULL\n", __func__);

    char *pt = t;

    while (*s)
    {
	if (!isgarbage(*s))
	    *pt++ = *s++;
	else
	{
	    *pt++ = ' ';
	    while (*s != '\0' && isgarbage(*s))
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
    return trim(t);
}

CaseTree *buildTree(const char *s)
{
    char *t = strdup(s); /* this never gets freed! */
    char *rulename = NULL;
    char *c, *sc, *es, *x;
    c = sc = es = x = NULL;
    CaseTree *ct = (CaseTree *)malloc(sizeof(CaseTree));
    if (ct == NULL) errExit("[%s] malloc returned NULL\n", __func__);

    char *temp = t;
    if ((t = strstr(temp, SC)) == NULL)
    { /* in this case there is no "Select Case" in the data */
	printf("Warning in %s: there are no cases in the assumption, "
		"so user should have chosen fixed amount for assumption.\n"
		, __func__); 	
	ct->rule_index = -1; /* no rule */
	ct->cond = NULL;
	ct->expr = temp;
	ct->next = NULL;
	ct->child = NULL;
	return ct;
    }

    /* at the root there is a rule (f.e. age, cat, reg, ...) */
    t += strlen(SC); /* SC includes space ' ' at the end */
    rulename = t;
    for (int i = 0; *t != '\0' && *t != ' '; i++)
	t++;
    *t = '\0';
    ct->rule_index = setRule(rulename);
    if (-1 == ct->rule_index) 
	errExit("[%s] Unknown rule [%s]\n", __func__, rulename);

    for (CaseTree *pct = ct; pct != NULL; pct = pct->next)
    {
	t += strlen(C);
	pct->cond = t;

	x = strstr(t, X);
	sc = strstr(t, SC);

	if (sc == NULL)
	    t = x;
	else 
	    t = (x < sc ? x : sc);
	*(t - 1) = '\0';
	pct->expr = t;

	if (strncmp(t, SC, strlen(SC)) == 0)
	{
	    char *prevt = t++;

	    /* select cases can be nested, we need to find the final end select 
	       of this tree */
	    int nests = 1;

	    while (nests)
	    {
		sc = strstr(t, SC);
		es = strstr(t, ES);
		if (sc == NULL || sc > es)
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

	if (c == NULL || es < c)
	{
	    t = es;
	    pct->next = NULL;
	}
	else
	{
	    t = c;
	    if ((pct->next = (CaseTree *)malloc(sizeof(CaseTree))) == NULL)
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
	else if (strncmp(name, ruleset[i].name, n) == 0) 
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
    printf("%sselect case %s\n", tabs, ruleset[ct->rule_index].name);

    strcpy(temp, tabs); /* remember tabs used here because it will be the same 
			   at end of tree */

    cnt++; /* when cases start we are one level removed from top */
    while (ct != NULL)
    {
	strcpy(tabs, "");
	for (int i = 0; i < cnt; i++)
	    strcat(tabs, "\t");
	printf("%scase %s\n", tabs, ct->cond);

	cnt++; /* either a child or new tree is created, both need another 
		  tab */
	if (ct->child == NULL)
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
    if (ct != NULL)
    {
	freeTree(ct->child);	
	freeTree(ct->next);	
	free(ct);
    }
}

static int cmpnum(CaseTree *ct, const void *pf)
{
    double f = *((double *)pf);
    char tmp[strlen(ct->cond) + 1];
    strcpy(tmp, ct->cond);
    char *pt = tmp;

    int n = 1; /* number of conditions separated by ',' 
		  (there is atleast 1 condition) */
    while (*pt)
	if (*pt++ == ',')
	    n++;

    char *cond = strtok(tmp, ",");     
    while (n--)
    {
	/* in case there is a "TO" operator */
	char *to;
	if ((to = strstr(cond, "TO")) != NULL)
	{
	    while (*cond != '\0' && !isdigit(*cond))
		cond++; 
	    while (*to != '\0' && !isdigit(*to))
		to++;

	    if (f >= atof(cond) && f <= atof(to))
		return 1;
	}

	/* in case of "<" or "<=" */
	else if ((to = strchr(cond, '<')) != NULL)
	{
	    while (*cond != '\0' && !isdigit(*cond))
		cond++;

	    if (*++to == '=')
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
	else if ((to = strchr(cond, '>')) != NULL)
	{
	    while (*cond != '\0' && !isdigit(*cond))
		cond++;

	    if (*++to == '=')
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
	else if ((to = strstr(cond, "ELSE")) != NULL)
	    return 1;

	/* in case it's just a fixed amount */
	else
	{
	    while (*cond != '\0' && !isdigit(*cond))
		cond++;

	    if (f == atof(cond))
		return 1;
	}

	cond = strtok(NULL, ",");
    }
    return 0;
}

static int cmpstr(CaseTree *ct, const void *s)
{
    char tmp[strlen(ct->cond)+1];
    strcpy(tmp, ct->cond);
    char *pt = tmp;

    int n = 1; /* number of conditions separated by ',' 
		  (there is atleast 1 condition) */
    while (*pt)
	if (*pt++ == ',')
	    n++;

    char *cond = strtok(tmp, ",");     
    while (n--)
    {
	if ((strstr(cond, "ELSE")) != NULL) /* if there is an else */
	    return 1;

	if ((cond = strinside(cond, "\"", "\"")) == NULL)
	    errExit("[%s] string in case should be "
		    "defined between quotes \"\"\n", __func__);

	if (strcmp(cond, (char *)s) == 0)
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
    for (CaseTree *pct = ct; pct != NULL; )
    {
	if (-1 == pct->rule_index) /* no rule, just an expression */
	{
	    while (*pct->expr != '\0' && !isdigit(*pct->expr))
		pct->expr++;
	    x = atof(pct->expr);
	    return x;
	}

	v = rule_data[pct->rule_index];
	cf = ruleset[pct->rule_index].cf;

	if (cf(pct, v))
	{
	    if ((pct->child) == NULL)
	    {
		while (*pct->expr != '\0' && !isdigit(*pct->expr))
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
