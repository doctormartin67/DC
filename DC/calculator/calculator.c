#include <stdio.h>
#include <stdlib.h>
#include "errorexit.h"
#include "libraryheader.h"

enum {STACK_SIZE = 128};

static unsigned topstack = 0;
static double stack[STACK_SIZE];

double eval_expr(const char s[static 1]);
static double addop(const char *s[static 1]);
static double multop(const char *s[static 1]);
static unsigned isop(int c);
static void push(double a);
static double pop(void);

int main(void)
{
	char buf[BUFSIZ];

	for (size_t i = 0; i < sizeof(buf); i++) {
		if (EOF == (buf[i] = getchar())) {
			buf[i] = '\0';
			break;
		}
	}
	printf("%f\n", eval_expr(buf));
	return 0;
}

/*
 * evaluates the expression given by s. Assumes the expression does not start
 * with white space
 */
double eval_expr(const char s[static 1])
{
	unsigned pushed = 0;
	double result = 0.0;

	push(addop(&s));
	pushed++;

	while (1) {
		switch (*s) {
			case '+' :
				s++;
				if (isop(*s)) die("two ops %c%c", *(s-1), *s);
				push(addop(&s));
				pushed++;
				break;
			case '-' : 
				s++;
				if (isop(*s)) die("two ops %c%c", *(s-1), *s);
				push(-addop(&s));
				pushed++;
				break;
			case '\0':
				goto evaluation;
				break;
		}
	}

evaluation:
	while (pushed--) {
		result += pop();
	}

	return result;
}

static double addop(const char *s[static 1])
{
	double m = multop(s);

	while (1) {
		switch (**s) {
			case '+' :
			case '-' : 
			case '\0':
				return m;
			case '*' :
				(*s)++;
				if (isop(**s))
					die("two ops %c%c", *(*s - 1), **s);
				m *= multop(s);
				break;
			case '/' :	
				(*s)++;
				if (isop(**s))
					die("two ops %c%c", *(*s - 1), **s);
				m /= multop(s);
				break;
		}
	}
}

static double multop(const char *s[static 1])
{
	const char *t = 0;

	while (isgarbage(**s)) (*s)++;    
	t = *s;

	if('+' == **s || '-' == **s) (*s)++;

	if (!isdigit(**s)) die("Invalid operator [%s]", t);

	while (isdigit(**s)) (*s)++;

	if ('.' == **s) {
		(*s)++;
		if (!isdigit(**s)) die("Invalid operator [%s]", t);
	}

	while (isdigit(**s)) (*s)++;
	while (isgarbage(**s)) (*s)++;    

	return atof(t);
}

static unsigned isop(int c)
{
	return '+' == c || '-' == c || '*' == c || '/' == c;
}

static void push(double a)
{
	if (STACK_SIZE - 1 < topstack) {
		die("stack overflow");
	} else {
		stack[topstack++] = a;
	}
	printf("%f pushed\n", a);
}

static double pop(void)
{
	if (0 == topstack) {
		die("stack underflow");
	} else {
		printf("%f popped\n", stack[topstack - 1]);
		return stack[--topstack];
	}
	return 0.0;
}
