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

#include "common.c"
#include "lex.c"
#include "ast.c"
#include "print.c"
#include "parse.c"

void test_buf(void)
{
	unsigned *buf = 0;
	assert(buf_len(buf) == 0);
	unsigned n = 1024;
	for (unsigned i = 0; i < n; i++) {
		buf_push(buf, i);
	}
	assert(buf_len(buf) == n);
	for (size_t i = 0; i < buf_len(buf); i++) {
		assert(buf[i] == i);
	}
	buf_free(buf);
	assert(buf == 0);
	assert(buf_len(buf) == 0);
	char *str = 0;
	buf_printf(str, "One: %d\n", 1);
	assert(strcmp(str, "One: 1\n") == 0);
	buf_printf(str, "Hex: 0x%x\n", 0x12345678);
	assert(strcmp(str, "One: 1\nHex: 0x12345678\n") == 0);
}

static Arena arena;
void test_arena(void)
{
	int *ptr = 0;
	ptr = arena_alloc(&arena, 64);
	*ptr = 4;
	assert(4 == *ptr);
	assert(ARENA_BLOCK_SIZE == arena.end - arena.ptr + 64);
	assert(arena.ptr - (char *)ptr == 64);
	assert(*arena.blocks == (char *)ptr);

	double *ptrd = 0;
	ptrd = arena_alloc(&arena, 256);
	*ptrd = 0.32;
	assert(0.32 == *ptrd);
	assert(ARENA_BLOCK_SIZE == arena.end - arena.ptr + 64 + 256);
	assert(arena.ptr - (char *)ptrd == 256);
	assert(buf_len(arena.blocks) == 1);

	arena_free(&arena);	
}


void test_map(void) {
	Map map = {0};
	enum { N = 1024 };
	for (size_t i = 1; i < N; i++) {
		map_put(&map, (void *)i, (void *)(i+1));
	}
	for (size_t i = 1; i < N; i++) {
		void *val = map_get(&map, (void *)i);
		assert(val == (void *)(i+1));
	}
}

void test_str_intern(void)
{
	const char x[] = "hello";
	const char y[] = "hello";
	const char z[] = "hello!";
	const char a[] = "shello";
	assert(x != y);
	assert(str_intern(x) == str_intern(y));
	assert(str_intern(x) != str_intern(z));
	assert(str_intern(y) != str_intern(a));
	assert(str_intern(z) != str_intern(a));
}

void test_keywords(void)
{
    init_keywords();
    assert(is_keyword_name(first_keyword));
    assert(is_keyword_name(last_keyword));
    for (const char **it = keywords; it != buf_end(keywords); it++) {
        assert(is_keyword_name(*it));
    }
    assert(!is_keyword_name(str_intern("foo")));
}

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) \
		&& match_token(TOKEN_NAME))
#define assert_token_int(x) assert(token.int_val == (x) \
		&& match_token(TOKEN_INT))
#define assert_token_float(x) assert(token.float_val == (x) \
		&& match_token(TOKEN_FLOAT))
#define assert_token_str(x) assert(strcmp(token.str_val, (x)) == 0 \
		&& match_token(TOKEN_STR))
#define assert_token_eof() assert(is_token(0))

void test_lex(void)
{
	test_keywords();
	assert(str_intern("for") == for_keyword);

	// Integer literal tests
	init_stream(0, "0 2147483647 042 1111");
	assert_token_int(0);
	assert_token_int(2147483647);
	assert_token_int(42);
	assert_token_int(1111);
	assert_token_eof();

	// Float literal tests
	init_stream(0, "3.14 .123 42.");
	assert_token_float(3.14);
	assert_token_float(.123);
	assert_token_float(42.);
	assert_token_eof();

	// String literal tests
	init_stream(0, "\"foo\" \"a\\nb\"");
	assert_token_str("foo");
	assert_token_str("a\\nb");
	assert_token_eof();

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
	const char *decls[] = {
		"dim x as Integer",
	};
	for (const char **it = decls; it != decls + sizeof(decls)/sizeof(*decls); it++) {
		init_stream(0, *it);
		Decl *decl = parse_decl();
		printf("\n");
	}

}

int main(void)
{
	test_buf();
	test_arena();
	test_lex();
	test_map();
	test_str_intern();
	test_expr();
	test_print();
	test_parse();
	return 0;
}
