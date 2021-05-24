#ifndef INTERPRETER
#define INTERPRETER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libraryheader.h"

#define C "CASE "
#define SC "SELECT CASE "
#define ES "END SELEC67"
#define X "X"

enum {RULESIZE = 3};

typedef struct casetree {
    char rule[RULESIZE + 1]; // f.e. AGE, CAT, REG
    char *cond; // condition, f.e. Is < 40, "WC", 12220, ... used to test the rule against
    char *expr; /* expression, f.e. x = 0.01, this can also be a select case because select cases 
		   can be nested and so the expression needs to be reevaluated */
    struct casetree *next; /* next case in current select case */
    struct casetree *child; /* select cases can be nested, 
			this points to first case of nested select case */
} CaseTree;

char *strclean(const char *);
double interpret(CaseTree *ct, double age, char *reg, char *cat);
CaseTree *buildTree(const char *);
void printTree(CaseTree *ct);

#endif
