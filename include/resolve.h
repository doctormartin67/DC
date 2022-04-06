#ifndef RESOLVE_H_
#define RESOLVE_H_

#include "ast.h"
#include "common.h"
#include "type.h"

/*
 * defined in type.c
 */
extern Type *type_boolean;
extern Type *type_int;
extern Type *type_double;
extern Type *type_string;

typedef enum SymKind {
	SYM_NONE,
	SYM_DIM,
	SYM_FUNC,
	SYM_TYPE,
} SymKind;

typedef struct Sym {
	const char *name;
	SymKind kind;
	double_func *func;
	unsigned is_project;
	struct {
		Type *type;
		Val val;
		struct {
			unsigned loop;
			double val;
		} project;
	};
} Sym;

typedef struct Operand {
	Type *type;
	unsigned is_lvalue;
	unsigned is_const;
	Val val;
} Operand;

enum {
	MAX_SYMS = 1024
};

Sym *sym_get(const char *name);
unsigned sym_push_var(const char *name, Type *type, Val val);
void syms_reset(void);
void resolve_stmts(Stmt **stmts);
void print_sym(Sym *sym);
void print_syms(void);

#endif
