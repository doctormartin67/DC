/* This file defines the error functions for the building of the Select Case
   tree. */

#include "interpreter.h"

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
    "Select Case has no rule"
};

/* Simple function to set the static variable to given value. */
void setterrno(TreeError te) { terrno = te; }

/* Simple function to get the static variable. */
TreeError getterrno(void) { return terrno; }

const char *strterror(TreeError te)
{
    return strterrors[te];
}
