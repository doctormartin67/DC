/* 
 * header file for interpreter.c
 */

#ifndef INTERPRETER
#define INTERPRETER

/* 
 * key words for Select Case in VBA
 */
#define C "CASE"
#define SC "SELECT CA67"
#define ES "END SELEC67" /* 67 is explained in interpreter.c:strclean.c */
#define X "X" /* An expression is of the form x = ... */
#define E "ELSE"

/* 
 * The indices of each rule within the ruleset array declared below. When a new
 * rule is added by the programmer, don't forget to add a rule to the ruleset 
 * array and also to the rule data that will be used as input.
 */
enum {AGE, REG, CAT, RULE_AMOUNT};

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

/* 
 * A rule is used to check the condition in the tree against the data that is
 * in the excel file for the affiliate. If there is a match then that 
 * expression is taken for the interpret function to return.
 */
struct rule {
	char *name;
	Cmpfunc *cf;
	void *data;
};

extern struct rule ruleset[RULE_AMOUNT];

char *strclean(const char *);
struct casetree *plantTree(const char *);
double interpret(const struct casetree ct[static 1],
		const void *rule_data[static RULE_AMOUNT]);
void printTree(const struct casetree ct[static 1]);
void chopTree(struct casetree *ct);

#endif
