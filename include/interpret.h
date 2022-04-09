#ifndef INTERPRET_H_
#define INTERPRET_H_

#include "common.h"
#include "type.h"
#include "excel.h"

typedef struct Interpreter {
	const char *code;
	const Database *db;
	int project_years;
	size_t num_record;
	TypeKind return_type;
	Sym *syms;
	size_t num_syms;
} Interpreter;

Val interpret(Interpreter *);
Interpreter *new_interpreter(const char *code, const Database *db,
		int project_years, size_t num_record, TypeKind return_type);
const Interpreter *get_interpreter(void);
void interpreter_free(Interpreter *i);
void add_user_sym(Sym *sym);
void add_builtin_int(const char *name, int i);
void add_builtin_boolean(const char *name, bool b);
void add_builtin_double(const char *name, double d);
void add_builtin_string(const char *name, const char *s);

#endif
