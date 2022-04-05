#ifndef AST_H_
#define AST_H_

#include <stdbool.h>
#include <stddef.h>
#include "lex.h"
#include "common.h"

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Typespec Typespec;

typedef struct StmtBlock {
	SrcPos pos;
	Stmt **stmts;
	size_t num_stmts;
} StmtBlock;

/*
 * TYPESPEC_PROJECT was created so that the user can create variable with a
 * 'projection' qualifier so that the variable will be projected each loop
 * according to the given qualifier
 */

typedef enum TypespecKind {
	TYPESPEC_NONE,
	TYPESPEC_NAME,
	TYPESPEC_PROJECT,
	TYPESPEC_FUNC,
} TypespecKind;

struct Typespec {
	TypespecKind kind;
	SrcPos pos;
	Typespec *base;
	union {
		const char *name;
		struct {
			Typespec **args;
			size_t num_args;
		} func;
		struct {
			Expr *expr;
		} project;
	};
};

typedef enum ExprKind {
	EXPR_NONE,
	EXPR_PAREN,
	EXPR_BOOLEAN,
	EXPR_INT,
	EXPR_FLOAT,
	EXPR_STR,
	EXPR_NAME,
	EXPR_CALL,
	EXPR_INDEX,
	EXPR_UNARY,
	EXPR_BINARY,
} ExprKind;

struct Expr {
	ExprKind kind;
	SrcPos pos;
	union {
		struct {
			Expr *expr;
		} paren;
		struct {
			bool val;
		} boolean_lit;
		struct {
			unsigned long long val;
		} int_lit;
		struct {
			const char *start;
			const char *end;
			double val;
		} float_lit;
		struct {
			const char *val;
		} str_lit;
		const char *name;
		struct {
			TokenKind op;
			Expr *expr;
		} unary;
		struct {
			TokenKind op;
			Expr *left;
			Expr *right;
		} binary;
		struct {
			Expr *expr;
			Expr **args;
			size_t num_args;            
		} call;
		struct {
			Expr *expr;
			Expr *index;
		} index;
	};
};

typedef struct ElseIf {
	Expr *cond;
	StmtBlock block;
} ElseIf;

typedef enum PatternKind {
	PATTERN_NONE,
	PATTERN_LIT,
	PATTERN_TO,
	PATTERN_IS,
} PatternKind;

typedef struct SelectCasePattern {
	PatternKind kind;
	union {
		Expr *expr;
		struct {
			Expr *start;
			Expr *end;
		} to_pattern;
		struct {
			TokenKind op;
			Expr *expr;
		} is_pattern;
	};
} SelectCasePattern;

typedef struct SelectCase {
	SelectCasePattern *patterns;
	size_t num_patterns;
	unsigned is_default;
	StmtBlock block;
} SelectCase;

typedef struct Dim {
	const char *name;
	Typespec *type;
} Dim;

typedef enum StmtKind {
	STMT_NONE,
	STMT_DECL,
	STMT_BLOCK,
	STMT_IF,
	STMT_WHILE,
	STMT_DO_WHILE,
	STMT_DO_UNTIL,
	STMT_DO_WHILE_LOOP,
	STMT_DO_UNTIL_LOOP,
	STMT_FOR,
	STMT_SELECT_CASE,
	STMT_ASSIGN,
	STMT_DIM,
	STMT_EXPR,
} StmtKind;

struct Stmt {
	StmtKind kind;
	SrcPos pos;
	union {
		Expr *expr;
		struct {
			Expr *cond;
			StmtBlock then_block;
			ElseIf *elseifs;
			size_t num_elseifs;
			StmtBlock else_block;            
		} if_stmt;
		struct {
			Expr *cond;
			StmtBlock block;
		} while_stmt;
		struct {
			Expr *cond;
			StmtBlock block;
		} until_stmt;
		struct {
			Stmt *dim;
			Expr *cond;
			Stmt *next;
			Expr *step;
			StmtBlock block;
		} for_stmt;
		struct {
			Expr *expr;
			SelectCase *cases;
			size_t num_cases;            
		} select_case_stmt;
		StmtBlock block;
		struct {
			TokenKind op;
			Expr *left;
			Expr *right;
		} assign;
		struct {
			Dim *dims;
			size_t num_dims;
		} dim_stmt;
	};
};

void ast_free(void);

StmtBlock new_StmtBlock(SrcPos pos, Stmt **stmts, size_t num_stmts);
Typespec *new_typespec_name(SrcPos pos, const char *name);
Typespec *new_typespec_project(SrcPos pos, Typespec *base, Expr *expr);
Expr *new_expr(ExprKind kind, SrcPos pos);
Expr *new_expr_paren(SrcPos pos, Expr *expr);
Expr *new_expr_boolean(SrcPos pos, bool val);
Expr *new_expr_int(SrcPos pos, unsigned long long val);
Expr *new_expr_float(SrcPos pos, const char *start, const char *end,
		double val);
Expr *new_expr_str(SrcPos pos, const char *val);
Expr *new_expr_name(SrcPos pos, const char *name);
Expr *new_expr_call(SrcPos pos, Expr *expr, Expr **args, size_t num_args);
Expr *new_expr_unary(SrcPos pos, TokenKind op, Expr *expr);
Expr *new_expr_binary(SrcPos pos, TokenKind op, Expr *left, Expr *right);

Stmt *new_stmt_if(SrcPos pos, Expr *cond, StmtBlock then_block,
		ElseIf *elseifs, size_t num_elseifs, StmtBlock else_block);
Stmt *new_stmt_while(SrcPos pos, Expr *cond, StmtBlock block);
Stmt *new_stmt_do_while(SrcPos pos, Expr *cond, StmtBlock block);
Stmt *new_stmt_do_until(SrcPos pos, Expr *cond, StmtBlock block);
Stmt *new_stmt_do_while_loop(SrcPos pos, Expr *cond, StmtBlock block);
Stmt *new_stmt_do_until_loop(SrcPos pos, Expr *cond, StmtBlock block);
Stmt *new_stmt_for(SrcPos pos, Stmt *dim, Expr *cond, Stmt *next,
		StmtBlock block);
Stmt *new_stmt_select_case(SrcPos pos, Expr *expr, SelectCase *cases,
		size_t num_cases);
Stmt *new_stmt_assign(SrcPos pos, TokenKind op, Expr *left, Expr *right);
Stmt *new_stmt_dim(SrcPos pos, Dim *dims, size_t num_dims);
Stmt *new_stmt_expr(SrcPos pos, Expr *expr);

#endif
