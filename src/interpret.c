#include <assert.h>
#include "interpret.h"
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "type.h"
#include "resolve.h"


/*
 * the user can create variables that will increase with each loop the program
 * is run in. this global variable is set to the current loop that the program
 * is in
 */
unsigned projection_loop;

/*
 * the interpreter also can take in a database so that the user can access a
 * record of an affiliate
 */
const Database *db_index;
size_t record_number;

static Sym builtin_syms[MAX_SYMS];
static Sym *builtin_syms_end = builtin_syms;

/*
 * every interpreter will have a 'result' variable to be returned
 */
static const char *result = "result";


static void add_builtin_var(const char *name, Type *type, Val val)
{
	if (builtin_syms_end == builtin_syms + MAX_SYMS) {
		fatal("Too many symbols");
	}
	*builtin_syms_end++ = (Sym){
		.name = str_intern(name),
			.kind = SYM_DIM,
			.type = type,
			.val = val
	};
}

static void add_builtin_vars(void)
{
	for (Sym *it = builtin_syms_end; it != builtin_syms; it--) {
		Sym *sym = it - 1;
		if (!sym_push_var(sym->name, sym->type, sym->val)) {
			fatal("Variable '%s' already added", sym->name);
		}
	}
}

static void builtin_syms_reset(void)
{
	for (Sym *it = builtin_syms_end; it != builtin_syms; it--) {
		Sym *sym = it - 1;
		*sym = (Sym){0};
	}
	builtin_syms_end = builtin_syms;
}

void add_builtin_int(const char *name, int i)
{
	add_builtin_var(name, type_int, (Val){.i = i});
}

void add_builtin_boolean(const char *name, bool b)
{
	add_builtin_var(name, type_boolean, (Val){.b = b});
}

void add_builtin_double(const char *name, int d)
{
	add_builtin_var(name, type_double, (Val){.d = d});
}

void add_builtin_string(const char *name, const char *s)
{
	add_builtin_var(name, type_string, (Val){.s = str_intern(s)});
}

static void init_interpreter(const char *vba_code, TypeKind kind,
		unsigned loop, const Database *db, size_t num_record)
{
	init_keywords();
	init_stream(0, vba_code);
	init_builtin_types();
	init_builtin_funcs();

	projection_loop = loop;
	db_index = db;
	record_number = num_record;

	switch (kind) {
		case TYPE_INT:
			add_builtin_int(result, 0);
			break;
		case TYPE_BOOLEAN:
			add_builtin_boolean(result, false);
			break;
		case TYPE_DOUBLE:
			add_builtin_double(result, 0.0);
			break;
		case TYPE_STRING:
			add_builtin_string(result, "");
			break;
		default:
			fatal("Unknown type for variable '%s'", result);
			break;
	}
	add_builtin_vars();
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
	projection_loop = 0;
	clear_stream();
	builtin_syms_reset();
	syms_reset();
	ast_free();
}

Val interpret(const char *vba_code, TypeKind return_type, unsigned loop,
		const Database *db, size_t num_record)
{
	init_interpreter(vba_code, return_type, loop, db, num_record);
	Val val = parse_interpreter()->val;
	clear_interpreter();
	return val;
}
