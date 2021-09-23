/* 
 * header file for interpreter.c
 */

#ifndef INTERPRETER
#define INTERPRETER

#include "calculator.h"
/* 
 * key words for Select Case in VBA
 */
#define C "CASE"
#define SC "SELECT CA67"
#define ES "END SELEC67" /* 67 is explained in interpreter.c:strclean.c */
#define X "X" /* An expression is of the form x = ... */
#define E "ELSE"

/* 
 * A casetree struct consists of:
 * - the index of the rule that determines what needs to be checked
 * - condition, f.e. Is < 40, "WC", "12220", ... used to test the rule
 * - expression, f.e. x = 0.01, this can also be a select case because 
 *   select cases can be nested and so the expression needs to be 
 *   reevaluated 
 * - next case in current Select Case
 * - child that points to nested Select Case within a Case
 */
struct casetree {
	int rule_index;     
	char *cond;     
	char *expr;     
	struct casetree *next;
	struct casetree *child;
};

/* 
 * This defines a compare function because some rules are strings and some are
 * doubles
 */
typedef unsigned Cmpfunc(const struct casetree *ct, const void *);
extern Cmpfunc cmpnum;
extern Cmpfunc cmpstr;

char *strclean(const char *);
struct casetree *plantTree(const char *);
double interpret(const struct casetree ct[static 1],
		const void *const rule_data[const static VAR_AMOUNT]);
void printTree(const struct casetree ct[static 1]);
void chopTree(struct casetree *ct);

#endif
