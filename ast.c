#include "ast.h"

static Arena ast_arena;

static size_t ast_memory_usage;

void *ast_alloc(size_t size)
{
	assert(0 != size);
	void *ptr = arena_alloc(&ast_arena, size);
	memset(ptr, 0, size);
	ast_memory_usage += size;
	return ptr;
}

void *ast_dup(const void *src, size_t size)
{
	if (0 == size) {
		return 0;
	}
	void *ptr = arena_alloc(&ast_arena, size);
	memcpy(ptr, src, size);
	return ptr;
}

#define AST_DUP(x) ast_dup(x, num_##x * sizeof(*x))

StmtList new_stmt_list(SrcPos pos, Stmt **stmts, size_t num_stmts)
{
	return (StmtList){pos, AST_DUP(stmts), num_stmts};
}

Typespec *new_typespec(TypespecKind kind, SrcPos pos)
{
	Typespec *t = ast_alloc(sizeof(*t));
	t->kind = kind;
	t->pos = pos;
	return t;
}

Typespec *new_typespec_name(SrcPos pos, const char **names, size_t num_names)
{
	Typespec *t = new_typespec(TYPESPEC_NAME, pos);
	t->names = AST_DUP(names);
	t->num_names = num_names;
	return t;

}

Typespec *new_typespec_func(SrcPos pos, Typespec **args, size_t num_args,
		Typespec *ret, unsigned has_varargs)
{
	Typespec *t = new_typespec(TYPESPEC_FUNC, pos);
	t->func.args = AST_DUP(args);
	t->func.num_args = num_args;
	TODO(t->func.ret = ret);
	TODO(t->func.has_varargs = has_varargs);
	return t;
}

Decls *new_decls(Decl **decls, size_t num_decls)
{
	Decls *d = ast_alloc(sizeof(*d));
	d->decls = AST_DUP(decls);
	d->num_decls = num_decls;
	return d;
}

Decl *new_decl(DeclKind kind, SrcPos pos, const char *name)
{
	Decl *d = ast_alloc(sizeof(*d));
	d->kind = kind;
	d->pos = pos;
	d->name = name;
	return d;
}

Decl *new_decl_var(SrcPos pos, const char *name, Typespec *type, Expr *expr)
{
	Decl *d = new_decl(DECL_VAR, pos, name);
	d->var.type = type;
	d->var.expr = expr;
	return d;
}

Expr *new_expr(ExprKind kind, SrcPos pos)
{
	Expr *e = ast_alloc(sizeof(*e));
	e->kind = kind;
	e->pos = pos;
	return e;
}

Expr *new_expr_paren(SrcPos pos, Expr *expr)
{
	Expr *e = new_expr(EXPR_PAREN, pos);
	e->paren.expr = expr;
	return e;
}

Expr *new_expr_int(SrcPos pos, unsigned long long val)
{
	Expr *e = new_expr(EXPR_INT, pos);
	e->int_lit.val = val;
	TODO(e->int_lit.suffix = suffix);
	return e;
}

Expr *new_expr_float(SrcPos pos, const char *start, const char *end,
		double val)
{
	Expr *e = new_expr(EXPR_FLOAT, pos);
	e->float_lit.start = start;
	e->float_lit.end = end;
	e->float_lit.val = val;
	TODO(e->float_lit.suffix = suffix);
	return e;
}

Expr *new_expr_str(SrcPos pos, const char *val)
{
	Expr *e = new_expr(EXPR_STR, pos);
	e->str_lit.val = val;
	return e;
}

Expr *new_expr_name(SrcPos pos, const char *name)
{
	Expr *e = new_expr(EXPR_NAME, pos);
	e->name = name;
	return e;
}

Expr *new_expr_call(SrcPos pos, Expr *expr, Expr **args, size_t num_args)
{
	Expr *e = new_expr(EXPR_CALL, pos);
	e->call.expr = expr;
	e->call.args = AST_DUP(args);
	e->call.num_args = num_args;
	return e;
}

Expr *new_expr_index(SrcPos pos, Expr *expr, Expr *index)
{
	Expr *e = new_expr(EXPR_INDEX, pos);
	e->index.expr = expr;
	e->index.index = index;
	return e;
}

Expr *new_expr_unary(SrcPos pos, TokenKind op, Expr *expr)
{
	Expr *e = new_expr(EXPR_UNARY, pos);
	e->unary.op = op;
	e->unary.expr = expr;
	return e;
}

Expr *new_expr_binary(SrcPos pos, TokenKind op, Expr *left, Expr *right)
{
	Expr *e = new_expr(EXPR_BINARY, pos);
	e->binary.op = op;
	e->binary.left = left;
	e->binary.right = right;
	return e;
}

Stmt *new_stmt(StmtKind kind, SrcPos pos)
{
	Stmt *s = ast_alloc(sizeof(*s));
	s->kind = kind;
	s->pos = pos;
	return s;
}

Stmt *new_stmt_label(SrcPos pos, const char *label)
{
	Stmt *s = new_stmt(STMT_LABEL, pos);
	s->label = label;
	return s;
}

Stmt *new_stmt_decl(SrcPos pos, Decl *decl)
{
	Stmt *s = new_stmt(STMT_DECL, pos);
	s->decl = decl;
	return s;
}

Stmt *new_stmt_block(SrcPos pos, StmtList block) {
	Stmt *s = new_stmt(STMT_BLOCK, pos);
	s->block = block;
	return s;
}

Stmt *new_stmt_if(SrcPos pos, Expr *cond, StmtList then_block,
		ElseIf *elseifs, size_t num_elseifs, StmtList else_block)
{
	Stmt *s = new_stmt(STMT_IF, pos);
	s->if_stmt.cond = cond;
	s->if_stmt.then_block = then_block;
	s->if_stmt.elseifs = AST_DUP(elseifs);
	s->if_stmt.num_elseifs = num_elseifs;
	s->if_stmt.else_block = else_block;
	return s;
}

Stmt *new_stmt_while(SrcPos pos, Expr *cond, StmtList block)
{
	Stmt *s = new_stmt(STMT_WHILE, pos);
	s->while_stmt.cond = cond;
	s->while_stmt.block = block;
	return s;
}

Stmt *new_stmt_do_while(SrcPos pos, Expr *cond, StmtList block)
{
	Stmt *s = new_stmt(STMT_DO_WHILE, pos);
	s->while_stmt.cond = cond;
	s->while_stmt.block = block;
	return s;
}

Stmt *new_stmt_do_until(SrcPos pos, Expr *cond, StmtList block)
{
	Stmt *s = new_stmt(STMT_DO_UNTIL, pos);
	s->while_stmt.cond = cond;
	s->while_stmt.block = block;
	return s;
}

Stmt *new_stmt_for(SrcPos pos, Stmt *init, Expr *cond, Stmt *next,
		StmtList block)
{
	Stmt *s = new_stmt(STMT_FOR, pos);
	s->for_stmt.init = init;
	s->for_stmt.cond = cond;
	s->for_stmt.next = next;
	s->for_stmt.block = block;
	return s;
}

Stmt *new_stmt_select_case(SrcPos pos, Expr *expr, SelectCase *cases,
		size_t num_cases)
{
	Stmt *s = new_stmt(STMT_SELECT_CASE, pos);
	s->select_case_stmt.expr = expr;
	s->select_case_stmt.cases = AST_DUP(cases);
	s->select_case_stmt.num_cases = num_cases;
	return s;
}

Stmt *new_stmt_assign(SrcPos pos, TokenKind op, Expr *left, Expr *right)
{
	Stmt *s = new_stmt(STMT_ASSIGN, pos);
	s->assign.op = op;
	s->assign.left = left;
	s->assign.right = right;
	return s;
}

Stmt *new_stmt_expr(SrcPos pos, Expr *expr)
{
	Stmt *s = new_stmt(STMT_EXPR, pos);
	s->expr = expr;
	return s;
}

#undef AST_DUP
