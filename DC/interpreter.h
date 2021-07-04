#ifndef INTERPRETER
#define INTERPRETER
#include "libraryheader.h"

#define C "CASE "
#define SC "SELECT CASE "
#define ES "END SELEC67"
#define X "X"

enum {RULESIZE = 3};

typedef struct
{
    char rule[RULESIZE + 1]; // f.e. AGE, CAT, REG
    char *cond; // condition, f.e. Is < 40, "WC", 12220, ... used to test the rule against
    char *expr; /* expression, f.e. x = 0.01, this can also be a select case because select cases 
		   can be nested and so the expression needs to be reevaluated */
    struct casetree *next; /* next case in current select case */
    struct casetree *child; /* select cases can be nested, 
			this points to first case of nested select case */
} CaseTree;

typedef int Cmpfunc(CaseTree *ct, const void *);

char *strclean(const char *);
double interpret(CaseTree *ct, double age, const char *reg, const char *cat);
CaseTree *buildTree(const char *);
void printTree(CaseTree *ct);

#endif
