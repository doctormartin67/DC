#include <stdio.h>
#include <assert.h>
#include "interpret.h"
#include "common.h"
#include "lex.h"
#include "type.h"
#include "resolve.h"
#include "helperfunctions.h"

Interpreter *interpreter;

static Sym **builtin_syms;
static Map interpreters;
extern Arena sym_arena;

/*
 * every interpreter will have a 'result' variable to be returned
 */
static const char *result = "result";

static Interpreter *new_interpreter(const char *code, const Database *db,
		int project_years, size_t num_record, TypeKind return_type)
{
	Interpreter *i = jalloc(1, sizeof(*i));
	i->code = code;
	i->db = db;
	i->project_years = project_years;
	i->num_record = num_record;
	i->return_type = return_type;
	i->builtin_syms = builtin_syms;
	i->properties.is_init = 1;
	return i;
}

static Sym *new_sym_builtin(const char *name, SymKind kind, Type *type,
		Val val)
{
	Sym *sym = arena_alloc(&sym_arena, sizeof(*sym));
	sym->name = name;
	sym->kind = kind;
	sym->type = type;
	sym->val = val;
	return sym;
}

static Sym *sym_get_builtin(const char *name)
{
	for (size_t i = 0; i < buf_len(builtin_syms); i++) {
		Sym *sym = builtin_syms[i];
		if (sym->name == name) {
			return sym;
		}
	}
	return 0;
}

static void add_builtin_var(const char *name, Type *type, Val val)
{
	Sym *sym = sym_get_builtin(name);
	if (sym) {
		sym->type = type;
		sym->val = val;
		return;
	}
	sym = new_sym_builtin(name, SYM_DIM, type, val);
	buf_push(builtin_syms, sym);
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

static void add_builtin_result(void)
{
	switch (interpreter->return_type) {
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
			die("Unknown type for variable '%s'", result);
			break;
	}
}

static void init_interpreter(void)
{
	init_builtin_types();
	init_builtin_funcs();
}

static void parse_interpreter(void)
{
	assert(interpreter);
	init_keywords();
	init_stream(0, interpreter->code);
	add_builtin_result();
	Stmt **stmts = interpreter->stmts;
	if (!stmts) {
		stmts = parse_stmts();
		interpreter->stmts = stmts;
	}
	resolve_stmts(stmts);
	for (size_t i = 0; i < buf_len(stmts); i++) {
		assert(stmts[i]);
	}
}

static void initial_parse(const char *code, const Database *db,
		int project_years, size_t num_record, TypeKind return_type)
{
	Interpreter *i = new_interpreter(code, db, project_years,
			num_record, return_type);
	map_put(&interpreters, code, i);
	assert(i);
	interpreter = i;
	init_interpreter();
	parse_interpreter();
	interpreter->result = sym_get_builtin(result)->val;
}

static void update_interpreter(const Database *db, int project_years,
		size_t num_record, TypeKind return_type)
{
	interpreter->db = db;
	interpreter->project_years = project_years;
	interpreter->num_record = num_record;
	interpreter->return_type = return_type;
}

Val interpret(const char *code, const Database *db, int project_years,
		size_t num_record, TypeKind return_type)
{
	Interpreter *i = map_get(&interpreters, code);
	if (!i) {
		result = str_intern(result);
		initial_parse(code, db, project_years, num_record,
				return_type);
	} else {
		interpreter = i;
		i->properties.is_init = 0;
		if (i->properties.has_project || i->properties.has_builtins) {
			update_interpreter(db, project_years, num_record,
					return_type);
			parse_interpreter();
			interpreter->result = sym_get_builtin(result)->val;
		}
	}
	return interpreter->result;
}
