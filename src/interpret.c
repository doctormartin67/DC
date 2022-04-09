#include <assert.h>
#include "interpret.h"
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "type.h"
#include "resolve.h"
#include "helperfunctions.h"

static Interpreter *interpreter;
static Sym **user_defined_syms;

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

void add_builtin_double(const char *name, double d)
{
	add_builtin_var(name, type_double, (Val){.d = d});
}

void add_builtin_string(const char *name, const char *s)
{
	add_builtin_var(name, type_string, (Val){.s = str_intern(s)});
}

static void init_interpreter(Interpreter *i)
{
	init_keywords();
	init_stream(0, i->code);
	init_builtin_types();
	init_builtin_funcs();

	interpreter = i;

	switch (i->return_type) {
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
	for (size_t i = 0; i < buf_len(user_defined_syms); i++) {
		buf_push(interpreter->syms, *user_defined_syms[i]);
		interpreter->num_syms++;
	}
	return sym_get(result);
}

static void clear_interpreter(void)
{
	interpreter = 0;
	clear_stream();
	builtin_syms_reset();
	syms_reset();
	buf_free(user_defined_syms);
	ast_free();
}

Val interpret(Interpreter *i)
{
	init_interpreter(i);
	Val val = parse_interpreter()->val;
	clear_interpreter();
	return val;
}

Interpreter *new_interpreter(const char *code, const Database *db,
		int project_years, size_t num_record, TypeKind return_type)
{
	Interpreter *i = jalloc(1, sizeof(*i));
	i->code = code;
	i->db = db;
	i->project_years = project_years;
	i->num_record = num_record;
	i->return_type = return_type;
	i->syms = 0;
	i->num_syms = 0;
	return i;
}

const Interpreter *get_interpreter(void)
{
	if (!interpreter) {
		die("interpreter not set");
	}
	return interpreter;
}

void interpreter_free(Interpreter *i)
{
	if (!i) {
		return;
	}
	buf_free(i->syms);
	free(i);
}

void add_user_sym(Sym *sym)
{
	assert(sym);
	if (!interpreter) {
		die("interpreter not set");
	}
	buf_push(user_defined_syms, sym);
}
