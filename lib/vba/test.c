#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h> /*defines HUGE_VAL */

#include "common.h"
#include "lex.h"
#include "type.h"
#include "ast.h"
#include "print_ast.h"
#include "parse.h"
#include "resolve.h"
#include "interpret.h"

#define PRINT_TEST_PARSE 0
#define PRINT_TEST_RESOLVE 1

void test_keywords(void)
{
	init_keywords();
	assert(is_keyword_name(first_keyword));
	assert(is_keyword_name(last_keyword));
	for (const char **it = keywords; it != buf_end(keywords); it++) {
		assert(is_keyword_name(*it));
	}
	assert(!is_keyword_name(str_intern("foo")));
	assert(str_intern("for") == for_keyword);
	buf_free(keywords);
}

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token_name() == str_intern(x) \
		&& match_token(TOKEN_NAME))
#define assert_token_int(x) assert(token_int_val() == (x) \
		&& match_token(TOKEN_INT))
#define assert_token_float(x) assert(token_float_val() == (x) \
		&& match_token(TOKEN_FLOAT))
#define assert_token_str(x) assert(strcmp(token_str_val(), (x)) == 0 \
		&& match_token(TOKEN_STR))
#define assert_token_eof() assert(is_token(0))

void test_lex(void)
{
	test_keywords();
	init_keywords();

	// Integer literal tests
	init_stream(0, "0 2147483647 042 1111");
	assert_token_int(0);
	assert_token_int(2147483647);
	assert_token_int(42);
	assert_token_int(1111);
	assert_token_eof();
	clear_stream();

	// Float literal tests
	init_stream(0, "3.14 .123 42.");
	assert_token_float(3.14);
	assert_token_float(.123);
	assert_token_float(42.);
	assert_token_eof();
	clear_stream();

	// String literal tests
	init_stream(0, "\"foo\" \"a\\nb\"");
	assert_token_str("foo");
	assert_token_str("a\\nb");
	assert_token_eof();
	clear_stream();

	// Operator tests
	init_stream(0, ": + < <= > >= - *");
	assert_token(TOKEN_COLON);
	assert_token(TOKEN_ADD);
	assert_token(TOKEN_LT);
	assert_token(TOKEN_LTEQ);
	assert_token(TOKEN_GT);
	assert_token(TOKEN_GTEQ);
	assert_token(TOKEN_SUB);
	assert_token(TOKEN_MUL);
	assert_token_eof();
	clear_stream();

	// Misc tests
	init_stream(0, "XY+(XY)_HELLO1,234+994");
	assert_token_name("XY");
	assert_token(TOKEN_ADD);
	assert_token(TOKEN_LPAREN);
	assert_token_name("XY");
	assert_token(TOKEN_RPAREN);
	assert_token_name("_HELLO1");
	assert_token(TOKEN_COMMA);
	assert_token_int(234);
	assert_token(TOKEN_ADD);
	assert_token_int(994);
	assert_token_eof();
	clear_stream();
}

#undef assert_token
#undef assert_token_name
#undef assert_token_int
#undef assert_token_float
#undef assert_token_str
#undef assert_token_eof

void test_expr(void)
{
	SrcPos pos = (SrcPos){0};
	Expr *e = new_expr(EXPR_INT, pos);
	assert(EXPR_INT == e->kind);
	Expr *ep = new_expr_paren(pos, e);
	assert(ep->paren.expr == e && EXPR_PAREN == ep->kind);
	Expr *e_int = new_expr_int(pos, 5);
	assert(5 == e_int->int_lit.val);
}

void test_parse(void)
{
	const char *ds[] = {
		"dim x as integer, y as double",
		"x = 4",
		"func(x)",
		"x = 4 + 3",
		"xyz = 2 + 4.3*1",
		"xyz = (2 + 4.3)*1",
		"func(x, xyz, 1+3)",
		"y = 1 / func(x, xyz, 1+3)",
		"if x = 1 then\n"
			"	x = 2\n"
			"	xyx = 2 + 3\n"
			"end if",
		"if y = 1*6 then\n"
			"	x = 2\n"
			"elseif (6 = 6) then\n"
			"	xyx = 2 + 3\n"
			"end if",
		"if y = 1*6 then\n"
			"	x = 2\n"
			"elseif (6 = 6) then\n"
			"	xyx = 2 + 3\n"
			"else\n"
			"	y = func(1, 2, 3*2)\n"
			"end if",
		"while x = y\n"
			"	xyz = xyz -1\n"
			"wend",
		"do while x = 1+2\n"
			"	x = y\n"
			"	xyz = 3 + 4/2*7\n"
			"loop",
		"do until x = 1+2\n"
			"	x = y\n"
			"	xyz = 3 + 4/2*7\n"
			"loop",
		"do\n"
			"	z = x + 1 - y + func(2,3, 0, x+y)\n"
			"loop while z = 1",
		"do\n"
			"	z = x + 1 - y + func(2,3, 0, x+y)\n"
			"loop until z = 1",
		"for i = 1 to 10\n"
			"x = x + i\n"
			"next",
		"for i = -1 to 10\n"
			"x = x + i\n"
			"next",
		"for i = -1 to -10 step -1\n"
			"x = x + i\n"
			"next",
		"for i = 1 to 10 step 2\n"
			"x = x + i\n"
			"next",
		"for i = 1 to 10 step 2\n"
			"x = x + i\n"
			"for j = 2 to 5\n"
			"x = 2*3\n"
			"next j\n"
			"next i",
		"for i = 1 to 10 step 2\n"
			"x = x + i\n"
			"next i",
		"select case x\n"
			"case \"hello\"\n"
			"y = 3+3\n"
			"x = 5*7\n"
			"end select",
		"select case x\n"
			"case \"hello\", \"world\"\n"
			"y = 3+3\n"
			"x = 5*7\n"
			"end select",
		"select case x\n"
			"case \"hello\", \"world\"\n"
			"y = 3+3\n"
			"x = 5*7\n"
			"case \"!\", \"I am joseph\"\n"
			"y = 3+3\n"
			"x = 5*7\n"
			"end select",
		"select case x\n"
			"case \"hello\", \"world\"\n"
			"y = 3+3\n"
			"x = 5*7\n"
			"case \"!\", \"I am joseph\"\n"
			"y = 3+3\n"
			"x = 5*7\n"
			"case else\n"
			"x = 2+3*7/2*func(3-2)\n"
			"end select",
		"select case x\n"
			"case 1:	y = 2 + 3\n"
			"case 2, 3,4:	y = 4 + 3\n"
			"end select",
		"select case x\n"
			"case 1 to 3:	y = 2 + 3\n"
			"case 2, 3,4:	y = 4 + 3\n"
			"end select",
		"select case x\n"
			"case is < 4:	y = xyz + 1\n"
			"case is>=4:	y = abc - 3*4\n"
			"end select",
		"if (x = 1 and y = 2 or z = 3 xor 7 = 7) then\n"
			"	x = 2^3+3^y*2\n"
			"	xyx = 2 + 3\n"
			"	xyx = 4 mod 3\n"
			"end if",
	};
	for (const char **it = ds; it != ds + sizeof(ds)/sizeof(*ds); it++) {
		init_stream(0, *it);
		Stmt *stmt = parse_stmt();
		assert(stmt);
#if PRINT_TEST_PARSE
		print_stmt(stmt);
		printf("\n");
#endif
		clear_stream();
	}
}

void test_resolve(void)
{
	const char *code = 
		"dim str as string\n"
		"dim str2 as string\n"
		"dim i as integer\n"
		"dim x as double\n"
		"dim bool as boolean\n"
		"bool = true or false or 2.2 and -2.3\n"
		"str = \"hello\""
		"if bool then\n"
			"x = -1.1 - 3/2\n"
		"elseif true then\n"
			"x = 1.2\n"
		"end if\n"
		"str2 = \"hel\" + \"lo\"\n"
		"bool = false\n"
		"bool = str = str2\n"
		;
	init_stream(0, code);
	init_builtin_types();
	Stmt **stmts = 0;
	stmts = parse_stmts();
	resolve_stmts(stmts);
	for (size_t i = 0; i < buf_len(stmts); i++) {
		assert(stmts[i]);
#if PRINT_TEST_RESOLVE
		print_stmt(stmts[i]);
		printf("\n");
#endif
	}
#if PRINT_TEST_RESOLVE
	print_syms();
#endif
	buf_free(stmts);
	clear_stream();
	syms_reset();

	enum {N = 1024 * 1024};
	for (int i = 0; i < N; i++) {
		interpret(code);
	}
	intern_free();
}

int main(void)
{
	test_lex();
	test_expr();
	test_parse();
	test_resolve();
	return 0;
}
