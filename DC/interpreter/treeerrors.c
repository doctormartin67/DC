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
