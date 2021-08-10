/* 
 * This file defines the error functions for the building of the Select Case
 * tree.
 */

#include "treeerrors.h"

/* 
 * Set to the error found (if any) while building the tree.
 */
static TreeError terrno = NOERR;

/* 
 * Array with error messages for tree errors
 */
const char *strterrors[] = 
{
    "No errors found in tree", 
    "Select Case without End Select", 
    "End Select without Select Case", 
    "No expression of the form 'x = *'",
    "Case without Select Case",
    "Select Case without Case",
    "Select Case has unknown rule",
    "Select Case has no rule", 
    "String condition without quotes \"\"", 
    "Conditions not separated by ','", 
    "Invalid condition",
    "Incorrect use of \"To\" operator",
    "Incorrect use of \"Is\" operator",
    "Incorrect use of \"Else\" operator"
};

void setterrno(TreeError te) { terrno = te; }

TreeError getterrno(void) { return terrno; }

const char *strterror(TreeError te)
{
    return strterrors[te];
}

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
    
    if (NULL == x)
    {
	setterrno(XERR);	
	return 0;
    }
    /* tree of the form 'x = *' is valid */
    if (NULL == c && NULL == sc && NULL == es)
	return 1;
    else if (NULL == sc && NULL != es)
    {
	setterrno(ESERR);
	return 0;
    }
    else if (NULL == sc && NULL != c)
    {
	setterrno(CERR);
	return 0;
    }
    else if (NULL == c)
    {
	setterrno(NOCERR);
	return 0;
    }
    else if (NULL == es)
    {
	setterrno(SCERR);
	return 0;
    }
    else if (NULL != c && NULL != sc && NULL != es)
    {
	if (c < sc)
	{
	    setterrno(CERR);
	    return 0;
	}
	else if (es < c)
	{
	    setterrno(NOCERR);
	    return 0;
	}
	else if (es < sc)
	{
	    setterrno(ESERR);
	    return 0;
	}
	else if (sc < c && c < es)
	    return 1;
	else
	{
	    printf("[%s] Warning: impossible part of function\n", __func__);
	    return 0;
	}
    }
    else
    {
	printf("[%s] Warning: impossible part of function\n", __func__);
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
     * n: number of conditions separated by ',' (there is atleast 1 condition)
     */
    int n = 1;
    const char *s = ct->cond;
    char t[strlen(s) + 1];
    char *pt = t;
    char *to = strstr(s, "TO");

    snprintf(t, sizeof(t), "%s", s);
    Cmpfunc *cf = ruleset[ct->rule_index].cf;

    while (*pt)
	if (',' == *pt++)
	    n++;
    
    pt = strtok(t, ",");

    /*
     * check whether the case is of the correct form when comparing numbers
     */
    if (cmpnum == cf)
    {
	while (n--)
	{
	    while (isgarbage(*pt))
		pt++; 

	    /*
	     * Case Is ...
	     */
	    if ('I' == *pt && 'S' == *(pt + 1))
	    {
		pt += 2;
		while (isgarbage(*pt))
		    pt++; 
		
		if ('<' == *pt || '>' == *pt)
		{
		    pt++;
		    if ('=' == *pt)
			pt++;

		    while (isgarbage(*pt))
			pt++;

		    if (!isdigit(*pt))
		    {
			setterrno(ISERR);
			return 0;
		    }

		    while (isdigit(*pt))
			pt++;

		    if ('.' == *pt)
		    {
			pt++;

			if (!isdigit(*pt))
			{
			    setterrno(ISERR);
			    return 0;
			}

			while (isdigit(*pt))
			    pt++;

			while (isgarbage(*pt))
			    pt++;

			if ('\0' != *pt)
			{
			    setterrno(ISERR);
			    return 0;
			}
		    }
		}
		else if (isdigit(*pt))
		{
		    while (isdigit(*pt))
			pt++;

		    while (isgarbage(*pt))
			pt++;

		    if ('\0' != *pt)
		    {
			setterrno(ISERR);
			return 0;
		    }
		}
		else
		{
		    setterrno(ISERR);
		    return 0;
		}
	    }
	    /*
	     * Case [0-9]+ To [0-9]+
	     */
	    else if (NULL != to)
	    {
		if (!isdigit(*pt))
		{
		    setterrno(TOERR);
		    return 0;
		}

		while (isdigit(*pt))
		    pt++;

		if ('.' == *pt)
		{
		    pt++;
		    while (isdigit(*pt))
			pt++;
		}

		while (isgarbage(*pt))
		    pt++;

		if ('T' == *pt && 'O' == *(pt + 1))
		{
		    pt += 2;

		    while (isgarbage(*pt))
			pt++;

		    while (isdigit(*pt))
			pt++;

		    if ('.' == *pt)
		    {
			pt++;
			while (isdigit(*pt))
			    pt++;
		    }

		    while (isgarbage(*pt))
			pt++;

		    if ('\0' != *pt)
		    {
			setterrno(TOERR);
			return 0;
		    }
		}
		else
		{
		    setterrno(TOERR);
		    return 0;
		}
	    }
	    else if (0 == strncmp(pt, E, strlen(E)))
	    {
		pt += strlen(E);

		while (isgarbage(*pt))
		    pt++;

		if ('\0' != *pt)
		{
		    setterrno(ELSERR);
		    return 0; 
		}
	    }
	    else if (NULL == to)
	    {
		while (isdigit(*pt))
		    pt++;

		while (isgarbage(*pt))
		    pt++;

		if ('\0' != *pt)
		{
		    setterrno(CONDERR);
		    return 0;
		}
	    }
	    else
		errExit("[%s] impossible condition reached\n", __func__);
	    
	    pt = strtok(NULL, ",");
	}
    }

    /*
     * check whether the case is of the correct form when comparing strings
     */
    else if (cmpstr == cf)
    {
	while (*s)
	{
	    while (isgarbage(*s))
		s++; 

	    /* all string conditions should be inside quotes */
	    if ('"' == *s++)
	    {
		while ('\0' != *s && '"' != *s)
		    s++;
		if ('\0' == *s)
		{
		    setterrno(QUOTERR); 
		    return 0;
		}
		else if ('"' == *s)
		{
		    s++;
		    while (isgarbage(*s))
			s++;

		    if (',' == *s)
			; /* OK, continue */
		    else if ('\0' == *s)
			break; /* OK, continue */
		    else if ('"' == *s)
		    {
			setterrno(SEPERR);			
			return 0;
		    }
		    else
		    {
			setterrno(QUOTERR); 
			return 0;
		    }
		}
		else
		    errExit("[%s] condition while loop impossible", __func__);
	    }
	    else if (0 == strncmp(ct->cond, E, strlen(E)))
		return 1;
	    else
	    {
		setterrno(QUOTERR);
		return 0;
	    }

	    s++;
	}
    }
    else
	errExit("[%s] condition while loop impossible (cf unknown)", __func__);

    return 1;
}
