/* This file defines the error functions for the building of the Select Case
   tree. */

#include "treeerrors.h"

/* Set to the error found (if any) while building the tree. */
static TreeError terrno = NOERR;

/* Array with error messages for tree errors */
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
    "Conditions not separated by ','"
};

/* Simple function to set the static variable to given value. */
void setterrno(TreeError te) { terrno = te; }

/* Simple function to get the static variable. */
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
    const char *s = ct->cond;
    Cmpfunc *cf = ruleset[ct->rule_index].cf;

    if (cmpnum == cf)
    {
	/* to do */	
    }
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
	    
	    /* continue checking condition */
	    s++;
	}
    }

    return 1;
}
