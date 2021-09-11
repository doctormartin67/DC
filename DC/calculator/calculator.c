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
#include <stdlib.h>
#include <math.h>
#include "errorexit.h"
#include "libraryheader.h"

enum {STACK_SIZE = 128};

static unsigned top = 0;
static double stack[STACK_SIZE];

double eval_expr(const char s[static 1]);
static double addop(const char *s[static 1]);
static double multop(const char *s[static 1]);
static unsigned isop(int c);
static void nextmultop(const char *s[static 1]);
static void push(double a);
static double pop(void);
unsigned valid_brackets(const char s[static 1]);

int main(void)
{
	char buf[BUFSIZ];

	for (size_t i = 0; i < sizeof(buf); i++) {
		if (EOF == (buf[i] = getchar())) {
			buf[i] = '\0';
			break;
		}
	}

	if (!valid_brackets(buf)) die("Incorrect brackets");

	printf("%f\n", eval_expr(buf));
	return 0;
}

/*
 * evaluates the expression given by s. Assumes the expression does not start
 * with white space
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
				while (isgarbage(*s)) s++;    
				if (isop(*s)) die("two ops %c%c", '+', *s);
				push(addop(&s));
				pushed++;
				break;
			case '-' : 
				s++;
				while (isgarbage(*s)) s++;    
				if (isop(*s)) die("two ops %c%c", '-', *s);
				push(-addop(&s));
				pushed++;
				break;
			case ')' :
				printf("pushed = %u\n", pushed);
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
			case ')' :
			case '\0':
				return m;
			case '*' :
				(*s)++;
				while (isgarbage(**s)) (*s)++;    
				if ('*' == **s || '/' == **s)
					die("two ops %c%c", '*', **s);
				m *= multop(s);
				break;
			case '/' :	
				(*s)++;
				while (isgarbage(**s)) (*s)++;    
				if ('*' == **s || '/' == **s)
					die("two ops %c%c", '/', **s);
				m /= multop(s);
				break;
			default :
				die("expected operator in between operands");
				break;
		}
	}
}

/*
 * finds the next multiplication operator and moves to the next operand
 * if it encounters an open bracket the process is started again recursively
 * and the pointer is moved to the next point just after the corresponding
 * closed bracket
 * if the operator is followed by a caret '^', it is taken to the power of the
 * next multiplication operator. This means that '^' will be from right to
 * left, f.e. 2^(1+2)^2 = 2^((1+2)^2) and not (2^(1+2))^2
 */
static double multop(const char *s[static 1])
{
	const char *t = 0;
	int sign = 1;
	double op = 0.0;

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
		while (isgarbage(**s)) (*s)++;

		t = *s;

		nextmultop(s);

		op = sign * eval_expr(t);			
	} else if (isdigit(**s)) {
		while (isdigit(**s)) (*s)++;

		if ('.' == **s) {
			(*s)++;
			if (!isdigit(**s)) die("Invalid operator");
			while (isdigit(**s)) (*s)++;
		}

		while (isgarbage(**s)) (*s)++;

		op = atof(t);
	} else {
		die("Invalid operator");
	}

	if ('^' == **s) {
		(*s)++;
		op = pow(op, multop(s)); /* CHECK FOR VALID OPERATORS??!! */
	}

	return op;
}

static unsigned isop(int c)
{
	return '+' == c || '-' == c || '*' == c || '/' == c;
}

/*
 * helper function for multop when an open bracket is encountered to move to
 * the corresponding closed bracket
 */
static void nextmultop(const char *s[static 1])
{
	unsigned brackets = 1;
	
	if (')' == **s) die("expected expression within brackets");

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
	printf("%f pushed\n", a);
}

static double pop(void)
{
	if (0 == top) {
		die("stack underflow");
	} else {
		printf("%f popped\n", stack[top - 1]);
		return stack[--top];
	}
	return 0.0;
}

/*
 * before the expression is evaulated, the brackets must be checked */
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
				if (!br--) return validity;
		}
		s++;
	}
	
	if (!br) validity = 1;
	else validity = 0;

	return validity;
}
