/*
 * primitive calculator that will be used to calculate the user input of
 * f.e. the premium of a pension plan. 
 * it uses a stack of addition operators, and with each addition operator
 * there can be many multiplication operators that first need to be multiplied
 * f.e.
 * 5 + 7 - 6*4
 * the stack will have the elements 5, 7, and -24, which will then be added to
 * produce the result.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "errorexit.h"
#include "libraryheader.h"
#include "calculator.h"
#include "treeerrors.h"

enum {STACK_SIZE = 256};
enum {MAX, MIN, FUNC_AMOUNT};

struct calculator_func {
	const char *const name;
	double (*func)(const char s[static 1]);
};

static unsigned top = 0;
static double stack[STACK_SIZE];

static double addop(const char *s[static 1]);
static double multop(const char *s[static 1]);
static void nextmultop(const char *s[static 1]);
static void push(double a);
static double pop(void);
static unsigned isfunc(const char s[static 1], unsigned *findex);
static unsigned infunc(const char s[static 1], const char sep[static 1]);
static double max(const char s[static 1]);
static double min(const char s[static 1]);
static unsigned isvar(const char s[static 1], unsigned *vindex);
static double cancel_calc(TreeError te);

static struct calculator_func func[FUNC_AMOUNT] = {
	[MAX] = {"MAX", max},
	[MIN] = {"MIN", min}
};

/*
 * var_set is an array which consists of the variables that the user
 * can use. They can be seen as the predetermined variables instead of the user
 * having to define them themselves.
 * data (NULL) will be set each time interpret is called because it can
 * be different each time
 * is_number is set to 1 if the variable is considered a number, 0 if it is
 * considered a string.
 * In the calculator all the variables are number, but the interpreter that
 * uses Select Case needs to check strings to determine which case to take
 */
struct variable var_set[VAR_AMOUNT] = 
{
	[VAR_AGE] = {"AGE", 1, .v.d = 0.0}, 
	[VAR_REG] = {"REG", 0, .v.s = ""}, 
	[VAR_CAT] = {"CAT", 0, .v.s = ""},
	[VAR_STATUS] = {"STATUS", 0, .v.s = ""},
	[VAR_SEX] = {"SEX", 1, .v.d = 0.0},
	[VAR_SAL] = {"SAL", 1, .v.d = 0.0},
	[VAR_PT] = {"PT", 1, .v.d = 0.0},
	[VAR_NDOA] = {"NDOA", 1, .v.d = 0.0},
	[VAR_NDOE] = {"NDOE", 1, .v.d = 0.0},
	[VAR_COMBINATION] = {"COMBINATION", 0, .v.s = ""},

	[VAR_LXMR] = {"LXMR", 1, .v.d = 0.0},
	[VAR_LXFR] = {"LXFR", 1, .v.d = 0.0},
	[VAR_LXMK] = {"LXMK", 1, .v.d = 0.0},
	[VAR_LXFK] = {"LXFK", 1, .v.d = 0.0},
	[VAR_LXFKP] = {"LXFKP", 1, .v.d = 0.0},
	[VAR_LXNIHIL] = {"LXNIHIL", 1, .v.d = 0.0},

	[VAR_CEIL1] = {"CEIL1", 1, .v.d = 0.0},
	[VAR_CEIL2] = {"CEIL2", 1, .v.d = 0.0},
	[VAR_CEIL3] = {"CEIL3", 1, .v.d = 0.0},
	[VAR_CEIL4] = {"CEIL4", 1, .v.d = 0.0},
	[VAR_CEIL5] = {"CEIL5", 1, .v.d = 0.0},
	[VAR_CEIL6] = {"CEIL6", 1, .v.d = 0.0},
	[VAR_CEIL7] = {"CEIL7", 1, .v.d = 0.0},
	[VAR_CEIL8] = {"CEIL8", 1, .v.d = 0.0},
	[VAR_CEIL9] = {"CEIL9", 1, .v.d = 0.0},
	[VAR_CEIL10] = {"CEIL10", 1, .v.d = 0.0},
};

void init_var(const union value parameters[VAR_AMOUNT])
{
	double d = 0.0;
	const char *s = "test";

	for (unsigned i = 0; i < VAR_AMOUNT; i++) {
		if (0 != parameters) {
			d = parameters[i].d;
			s = parameters[i].s;
		}

		if (var_set[i].is_number)
			var_set[i].v.d = d;
		else
			snprintf(var_set[i].v.s, sizeof(var_set[i].v.s),
					"%s", s);
	}
}

/*
 * evaluates the expression given by s.
 * The function pushes each addition operator to the stack.
 * This function has indirect recursion because it can be recalled in multop
 * when there are brackets to evaluate the brackets first.
 */
double eval_expr(const char s[static 1])
{
	unsigned pushed = 0;
	double result = 0.0;

	while (1) {
		switch (*s) {
			case '+' :
				s++;
				push(addop(&s));
				pushed++;
				break;
			case '-' : 
				s++;
				push(-addop(&s));
				pushed++;
				break;
			case ',' :
				if (0 == pushed)
					return cancel_calc(COMMAERR);
			case ')' :
			case '\0':
				goto evaluation;
				break;
			case '(':
			default :
				push(addop(&s));
				pushed++;
				break;
		}
	}

evaluation:
	while (pushed--) {
		result += pop();
	}

	return result;
}

/*
 * finds the next addition operator and moves the pointer to the next addition
 * operand f.e. if the string at this point is "5*3 + 6" it will return 15 and
 * the pointer will be at "+ 6" ready for the next operator to be pushed to the
 * stack
 */
static double addop(const char *s[static 1])
{
	double m = multop(s);

	while (1) {
		switch (**s) {
			case '+' :
			case '-' : 
			case ',' :
			case ')' :
			case '\0':
				return m;
			case '*' :
				(*s)++;
				while (isgarbage(**s)) (*s)++;    
				if ('*' == **s || '/' == **s)
					return cancel_calc(TWOOPERR);
				m *= multop(s);
				break;
			case '/' :	
				(*s)++;
				while (isgarbage(**s)) (*s)++;    
				if ('*' == **s || '/' == **s)
					return cancel_calc(TWOOPERR);
				m /= multop(s);
				break;
			default :
				return cancel_calc(OPERANDERR);
		}
	}
}

/*
 * finds the next multiplication operator and moves to the next operand
 * if it encounters an open bracket the process is started again recursively
 * and the pointer is moved to the next point just after the corresponding
 * closed bracket
 * if the operator is followed by a caret '^', it is taken to the power of the
 * next multiplication operator using recursion.
 * This means that '^' will be from right to left, 
 * f.e. 2^(1+2)^2 = 2^((1+2)^2) and not (2^(1+2))^2
 */
static double multop(const char *s[static 1])
{
	unsigned findex = 0;
	unsigned vindex = 0;
	int sign = 1;
	const char *t = 0;
	double op = 0.0;
	double var = 0.0;

	while (isgarbage(**s)) (*s)++;    
	t = *s;

	if ('+' == **s) {
		(*s)++;
	} else if ( '-' == **s) {
		sign = -1;
		(*s)++;
	}

	if ('(' == **s) {
		(*s)++;
		t = *s;
		nextmultop(s);

		op = sign * eval_expr(t);			

	} else if (isdigit(**s)) {

		while (isdigit(**s)) (*s)++;
		if ('.' == **s) {
			(*s)++;
			if (!isdigit(**s)) return cancel_calc(INVOPERR);
			while (isdigit(**s)) (*s)++;
		}
		while (isgarbage(**s)) (*s)++;

		op = atof(t);

	} else if (isfunc(*s, &findex)) {
		*s += strlen(func[findex].name);
		while (isgarbage(**s)) (*s)++;
		if ('(' != **s) return cancel_calc(FUNCBRERR);
		(*s)++;
		t = *s;
		nextmultop(s);

		op = sign * func[findex].func(t);

	} else if (isvar(*s, &vindex)) {
		*s += strlen(var_set[vindex].name);
		while (isgarbage(**s)) (*s)++;

		var = var_set[vindex].v.d;
		op = sign * var;

	} else {
		(*s)++;
		return cancel_calc(INVOPERR);
	}

	if ('^' == **s) {
		(*s)++;
		op = pow(op, multop(s));
	}

	return op;
}

/*
 * helper function for multop when an open bracket is encountered to move to
 * the corresponding closed bracket
 */
static void nextmultop(const char *s[static 1])
{
	unsigned brackets = 1;
	
	if (')' == **s) {
		cancel_calc(NOEXPRBRERR);
		return;
	}

	while (brackets) {
		switch (**s) {
			case '(' :
				brackets++;
				(*s)++;
				break;
			case ')' :
				brackets--;
				(*s)++;
				break;
			case '\0':
				if (brackets) die("Inconsistent brackets");
				break;
			default :
				(*s)++;
				break;
		}
	}
	while (isgarbage(**s)) (*s)++;
}

static void push(double a)
{
	if (STACK_SIZE - 1 < top) {
		die("stack overflow");
	} else {
		stack[top++] = a;
	}
}

static double pop(void)
{
	if (0 == top) {
		die("stack underflow");
	} else {
		return stack[--top];
	}
	return 0.0;
}

/*
 * before the expression is evaulated, the brackets must be checked
 */
unsigned valid_brackets(const char s[static 1])
{
	unsigned br = 0;
	unsigned validity = 0;

	while (*s) {
		switch (*s) {
			case '(' :
				br++;
				break;
			case ')' :
				if (!br--) {
					setterrno(INCBRERR);
					return validity;
				}
		}
		s++;
	}
	
	if (!br) validity = 1;
	else {
		setterrno(INCBRERR);
		validity = 0;
	}

	return validity;
}

/*
 * before the expression is evaluated, the ',' must be checked
 */
unsigned valid_separators(const char s[static 1])
{
	for (const char *sep = strchr(s, ','); sep; sep = strchr(sep, ',')) {
		if (!infunc(s, sep)) {
			setterrno(COMMAERR);
			return 0;
		}
		sep++;
	}

	return 1;
}

/*
 * checks whether the given separator lies within one of the functions in func
 * it checks whether there are brackets surrounding the separator and whether
 * there is a known function at the start
 */
static unsigned infunc(const char s[static 1], const char sep[static 1])
{
	unsigned left = 0, right = 0;
	int brackets = 0;
	const char *t = sep;

	while (t > s) {
		t--;
		switch (*t) {
			case '(':
				brackets++;
				break;
			case ')':
				brackets--;
				break;
			default : break;
		}

		if (1 == brackets) {
			if (t-- == s) return 0;
			while (t > s && isgarbage(*t)) t--;
			while (t > s && isalnum(*t)) t--;
			if (!isalnum(*t)) t++;

			if (isfunc(t, 0)) left = 1;
			else return 0;

			break;
		}
	}
	if (!left) return 0;

	t = sep;
	brackets = 0;

	while ('\0' != *t) {
		t++;
		switch (*t) {
			case '(':
				brackets++;
				break;
			case ')':
				brackets--;
				break;
			default : break;
		}

		if (-1 == brackets) {
			right = 1;
			break;
		}
	}

	if (!right) return 0;
	else return 1;
}

/*
 * returns 1 if the string starts with one of the functions defined in func,
 * 0 otherwise. It will also set *findex to the index of the function given in
 * func if findex is not NULL
 */
static unsigned isfunc(const char s[static 1], unsigned *findex)
{
	for (unsigned i = 0; i < FUNC_AMOUNT; i++) {
		if (0 == strncmp(s, func[i].name, strlen(func[i].name))) {
			if (findex) *findex = i;
			return 1;
		}
	}

	return 0;
}

/*
 * given a string excluding the open bracket at the start, find the maximum of
 * values separated by ','.
 * f.e. 5, 1, 3 will return 5
 */
static double max(const char s[static 1])
{
	double curr = 0;
	double m = eval_expr(s);

	for (const char *next = strchr(s, ','); next;
			next = strchr(next, ',')) {
		next++;
		if (')' == *next) return cancel_calc(COMMAERR);
		curr = eval_expr(next);
		m = MAX2(m, curr);
	}

	return m;
}

/*
 * given a string excluding the open bracket at the start, find the minumum of
 * values separated by ','.
 * f.e. 5, 1, 3 will return 1
 */
static double min(const char s[static 1])
{
	double curr = 0;
	double m = eval_expr(s);

	for (const char *next = strchr(s, ','); next;
			next = strchr(next, ',')) {
		next++;
		if (')' == *next) return cancel_calc(COMMAERR);
		curr = eval_expr(next);
		m = MIN2(m, curr);
	}

	return m;
}

/*
 * returns 1 if the string starts with one of the variables defined in var_set
 * and is a number, 0 otherwise. It will also set *vindex to the index of the
 * variable given in var_set if vindex is not NULL
 */
static unsigned isvar(const char s[static 1], unsigned *vindex)
{
	unsigned len = 0;
	unsigned slen = 0;
	const char *name = 0;
	for (unsigned i = 0; i < VAR_AMOUNT; i++) {
		name = var_set[i].name;
		len = strlen(name);
		slen = MIN2(len, strlen(s));
		if (0 == strncmp(s, name, len) && !isalnum(s[slen])) {
			if (!var_set[i].is_number) cancel_calc(NUMERR);
			if (vindex) *vindex = i;
			return 1;
		}
	}

	return 0;
}

static double cancel_calc(TreeError te)
{
	if (NOERR == getterrno()) setterrno(te);
	return 0.0;
}
