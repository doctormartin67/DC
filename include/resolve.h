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
	struct {
		Type *type;
		Val val;
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

void resolve_stmts(Stmt **stmts);
void print_syms(void);

#endif
