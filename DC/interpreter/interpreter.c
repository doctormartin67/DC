#include "interpreter.h"

static Cmpfunc cmpnum;
static Cmpfunc cmpstr;

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
	    while (isgarbage(*s))
		s++;
	}
    }
    *pt = '\0';

    upper(t); // make case insensitive

    /* END SELECT needs to be replaced with something else, because I need a unique identifier
       to find the beginning and end of a select case. If there is a case after end select, then
       strstr(s, "SELECT CASE") would find this when in fact this isn't the beginning of a select
       case, it's the end of the previous one. I chose "END SELEC67" */
    pt = t;
    t = replace(t, "END SELECT", ES);
    free(pt);
    return trim(t);
}

CaseTree *buildTree(const char *s)
{
    char *t = strdup(s); /* this never gets freed! */
    char *c, *sc, *es, *x;
    CaseTree *ct = (CaseTree *)malloc(sizeof(CaseTree));
    if (ct == NULL) errExit("[%s] malloc returned NULL\n", __func__);

    if ((t = strstr(t, SC)) == NULL)
    {
	printf("Warning in %s: there are no cases in the assumption, "
		"so user should have chosen fixed amount for assumption.\n"
		, __func__); 	
	free(ct);
	return NULL;
    }

    /* at the root there is a rule (f.e. age, cat, reg, ...) */
    t += strlen(SC);
    for (int i = 0; *t != ' ' && i < RULESIZE; i++)
	ct->rule[i] = *t++;
    ct->rule[RULESIZE] = '\0';

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

	    /* select cases can be nested, we need to find the final end select of this tree */
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
	    strcpy(pct->next->rule, pct->rule);
	}
	*(t - 1) = '\0';
    }

    return ct;
}

void printTree(CaseTree *ct)
{
    static int cnt = 0; /* count the tabs to be used */
    char tabs[32] = "";
    char temp[32] = ""; /* used for end of tree (end select) */

    for (int i = 0; i < cnt; i++)
	strcat(tabs, "\t");
    printf("%sselect case %s\n", tabs, ct->rule);

    strcpy(temp, tabs); /* remember tabs used here because it will be the same at end of tree */

    cnt++; /* when cases start we are one level removed from top */
    while (ct != NULL)
    {
	strcpy(tabs, "");
	for (int i = 0; i < cnt; i++)
	    strcat(tabs, "\t");
	printf("%scase %s\n", tabs, ct->cond);

	cnt++; /* either a child or new tree is created, both need another tab */
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

    int n = 1; // number of conditions separated by ',' (there is atleast 1 condition)
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
	    while (!isdigit(*cond))
		cond++; 
	    while (!isdigit(*to))
		to++;

	    if (f >= atof(cond) && f <= atof(to))
		return 1;
	}

	/* in case of "<" or "<=" */
	else if ((to = strchr(cond, '<')) != NULL)
	{
	    while (!isdigit(*cond))
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
	    while (!isdigit(*cond))
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
	    while (!isdigit(*cond))
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

    int n = 1; // number of conditions separated by ',' (there is atleast 1 condition)
    while (*pt)
	if (*pt++ == ',')
	    n++;

    char *cond = strtok(tmp, ",");     
    while (n--)
    {
	if ((strstr(cond, "ELSE")) != NULL) /* if there is an else */
	    return 1;

	if ((cond = strinside(cond, "\"", "\"")) == NULL)
	    errExit("[%s] string in case should be defined between quotes \"\"\n", __func__);

	if (strcmp(cond, (char *)s) == 0)
	{
	    free(cond);
	    return 1;
	}
	
	cond = strtok(NULL, ",");
    }
    return 0;
}

double interpret(CaseTree *ct, double age, const char *reg, const char *cat)
{
    double x = 0.0;
    Cmpfunc *cf;
    const void *v;
    for (CaseTree *pct = ct; pct != NULL; )
    {
	if (strcmp(pct->rule, "AGE") == 0)
	{
	    cf = cmpnum;
	    v = &age;
	}
	else if (strcmp(pct->rule, "REG") == 0) 
	{
	    cf = cmpstr;
	    v = reg;
	}
	else if (strcmp(pct->rule, "CAT") == 0)
	{
	    cf = cmpstr;
	    v = cat;
	}
	else 
	{
	    /* should never reach here because this should have been 
	       checked before program is run */
	    errExit("[%s]: Unknown rule \"%s\"\n", __func__, pct->rule);
	}

	if (cf(pct, v))
	{
	    if ((pct->child) == NULL)
	    {
		while (!isdigit(*pct->expr))
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
