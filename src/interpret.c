#include <assert.h>
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "type.h"
#include "resolve.h"

/*
 * every interpreter will have a 'result' variable to be returned
 */
static const char *result = "result";
static void init_interpreter(const char *vba_code, Type *type)
{
	init_stream(0, vba_code);
	init_builtin_types();
	sym_push_dim(result, type, 1);
}

static Sym *parse_interpreter(void)
{
	Stmt **stmts = 0;
	stmts = parse_stmts();
	resolve_stmts(stmts);
	for (size_t i = 0; i < buf_len(stmts); i++) {
		assert(stmts[i]);
	}
	buf_free(stmts);
	return sym_get(result);
}

static void clear_interpreter(void)
{
	clear_stream();
	syms_reset();
	ast_free();
}

Val interpret(const char *vba_code, Type *type)
{
	init_interpreter(vba_code, type);
	Val val = parse_interpreter()->val;
	clear_interpreter();
	return val;
}
