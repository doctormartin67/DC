#ifndef INTERPRET_H_
#define INTERPRET_H_

#include "parse.h"
#include "common.h"
#include "type.h"
#include "excel.h"

typedef struct Interpreter {
	const char *name;
	const char *code;
	const Database *db;
	int project_years;
	size_t num_record;
	TypeKind return_type;
	Stmt **stmts;
	Sym **local_syms;
	Sym **builtin_syms;
	Sym **builtin_type_syms;
	Sym **builtin_func_syms;
	Val result;
	struct {
		unsigned is_init;
		unsigned has_builtin;
		unsigned has_project;
	} properties;
} Interpreter;

Val interpret(const char *name, const char *code, const Database *db,
		int project_years, size_t num_record, TypeKind return_type);
const Interpreter *get_interpreter(void);
void interpreter_free(Interpreter *i);
void add_builtin_int(const char *name, int i);
void add_builtin_boolean(const char *name, bool b);
void add_builtin_double(const char *name, double d);
void add_builtin_string(const char *name, const char *s);
void print_syms(const Interpreter *ipr);

#endif
