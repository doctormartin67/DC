#include <assert.h>
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "type.h"
#include "resolve.h"

static void init_interpreter(const char *vba_code)
{
	init_stream(0, vba_code);
	init_builtin_types();
}

static Sym *parse_interpreter(const char *name)
{
	assert(name);
	Stmt **stmts = 0;
	stmts = parse_stmts();
	resolve_stmts(stmts);
	for (size_t i = 0; i < buf_len(stmts); i++) {
		assert(stmts[i]);
	}
	buf_free(stmts);
	return sym_get(name);
}

static void clear_interpreter(void)
{
	clear_stream();
	syms_reset();
	ast_free();
}

void interpret(const char *vba_code)
{
	init_interpreter(vba_code);
	(void)parse_interpreter("x");
	clear_interpreter();
}
