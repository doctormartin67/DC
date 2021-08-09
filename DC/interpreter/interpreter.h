#ifndef INTERPRETER
#define INTERPRETER

/* header file for interpreter.c */

#include "libraryheader.h"

/* key words for Select Case in VBA */
#define C "CASE "
#define SC "SELECT CA67 "
#define ES "END SELEC67" /* 67 is explained in interpreter.c:strclean.c */
#define X "X" /* An expression is of the form x = ... */

/* The indices of each rule within the ruleset array declared below. When a new 
   rule is added by the programmer, don't forget to add a rule to the ruleset 
   array and also to the rule data that will be used as input. */
enum rules {AGE, REG, CAT};

typedef struct casetree 
{
    int rule_index; /* the index of the rule determines what needs to be checked, 
			(will be set to one of the enum rules) for example:
			age, reglement, category, ... */
    char *cond; /* condition, f.e. Is < 40, "WC", 12220, ... used to test the 
		   rule */
    char *expr; /* expression, f.e. x = 0.01, this can also be a select case 
		   because select cases can be nested and so the expression 
		   needs to be reevaluated */
    struct casetree *next; /* next case in current select case */
    struct casetree *child; /* select cases can be nested, 
			this points to first case of nested select case */
} CaseTree;

/* This defines a compare function because some rules are strings and some are 
   doubles */
typedef int Cmpfunc(CaseTree *ct, const void *);

/* A Rule is used to check the condition in the tree against the data that is
   in the excel file for the affiliate. If there is a match then that 
   expression is taken for the interpret function to return. */
typedef struct 
{
    char *name;
    Cmpfunc *cf;
    void *data;
} Rule;

extern Rule ruleset[];

char *strclean(const char *);
CaseTree *buildTree(const char *);
int setRule(const char *);
double interpret(CaseTree *ct, const void *rule_data[]);
void printTree(CaseTree *ct);
void freeTree(CaseTree *ct);
/* In a Select Case statement we could be comparing strings or numbers,
   the two functions below are used to compare these two types. */
extern Cmpfunc cmpnum;
extern Cmpfunc cmpstr;

#endif
