#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include "interpret.h"
#include "resolve.h"
#include "excel.h"

extern Interpreter *interpreter;
Arena sym_arena;

static void print_sym_dim(Sym *sym)
{
	assert(SYM_DIM == sym->kind);
	switch (sym->type->kind) {
		case TYPE_BOOLEAN:
			if (sym->val.b)
				printf("%s: True\n", sym->name);
			else
				printf("%s: False\n", sym->name);
			break;
		case TYPE_INT:
			printf("%s: %d\n", sym->name, sym->val.i);
			break;
		case TYPE_DOUBLE:
			printf("%s: %f\n", sym->name, sym->val.d);
			break;
		case TYPE_STRING:
			printf("%s: %s\n", sym->name, sym->val.s);
			break;
		default:
			assert(0);
			break;
	}
}

void print_sym(Sym *sym)
{
	assert(sym);
	switch (sym->kind) {
		case SYM_TYPE:
		case SYM_FUNC:
			printf("%s\n", sym->name);
			break;
		case SYM_DIM:
			print_sym_dim(sym);	
			break;
		default:
			assert(0);
			break;
	}

}

#define PRINT(t) \
	printf(#t " syms:\n"); \
	for (size_t i = 0; i < buf_len(ipr->t##_syms); i++) { \
		print_sym(ipr->t##_syms[i]); \
	}

void print_syms(const Interpreter *ipr)
{
	PRINT(local);
	PRINT(builtin);
	PRINT(builtin_type);
	PRINT(builtin_func);
}

#undef PRINT

#define RETURN(t) \
	assert(interpreter); \
	for (size_t i = 0; i < buf_len(interpreter->t##_syms); i++) { \
		Sym *sym = interpreter->t##_syms[i]; \
		if (sym->name == name) { \
			return sym; \
		} \
	} \
	return 0;


static Sym *sym_get_local(const char *name)
{
	RETURN(local);
}

static Sym *sym_get_builtin(const char *name)
{
	RETURN(builtin);
}

static Sym *sym_get_type(const char *name)
{
	RETURN(builtin_type);
}

static Sym *sym_get_func(const char *name)
{
	RETURN(builtin_func);
}

#undef RETURN

static Sym *sym_get(const char *name)
{
	Sym *sym = sym_get_local(name);
	if (!sym) {
		sym = sym_get_builtin(name);
		if (!sym) {
			sym = sym_get_type(name);
			if (!sym) {
				sym = sym_get_func(name);
			}
		} else {
			interpreter->properties.has_builtins = 1;
		}
	}
	return sym;
}

static Sym *new_sym(const char *name, SymKind kind, Type *type)
{
	Sym *sym = arena_alloc(&sym_arena, sizeof(*sym));
	sym->name = str_intern(name);
	sym->kind = kind;
	sym->type = type;
	return sym;
}

static Sym *new_sym_func(const char *name, SymKind kind, Type *type,
		double_func func)
{
	Sym *sym = arena_alloc(&sym_arena, sizeof(*sym));
	sym->name = str_intern(name);
	sym->kind = kind;
	sym->func = func;
	sym->type = type;
	return sym;
}

static Sym *new_sym_project(const char *name, SymKind kind, Type *type,
		double val)
{
	Sym *sym = arena_alloc(&sym_arena, sizeof(*sym));
	sym->name = str_intern(name);
	sym->is_project = 1;
	sym->kind = kind;
	sym->type = type;
	sym->project.val = val;
	return sym;
}

unsigned sym_push_type(const char *name, Type *type)
{
	if (!interpreter->properties.is_init) {
		return 1;
	}
	if (sym_get_type(name)) {
		return 0;
	}
	Sym *sym = new_sym(name, SYM_TYPE, type);
	assert(interpreter);
	buf_push(interpreter->builtin_type_syms, sym);
	return 1;
}

unsigned sym_push_func(const char *name, Type *type, double_func func)
{
	if (!interpreter->properties.is_init) {
		return 1;
	}
	if (sym_get_func(name)) {
		return 0;
	}
	Sym *sym = new_sym_func(name, SYM_FUNC, type, func);
	assert(interpreter);
	buf_push(interpreter->builtin_func_syms, sym);
	return 1;
}

static Operand resolve_expr(Expr *expr);
static unsigned convert_operand(Operand *operand, Type *type);
static unsigned sym_push_project(const char *name, Type *type,
		unsigned eval_stmt, Typespec *typespec)
{
	if (!interpreter->properties.is_init) {
		return 1;
	}
	if (sym_get(name)) {
		return 0;
	}
	interpreter->properties.has_project = 1;
	Operand operand = resolve_expr(typespec->project.expr);
	if (!convert_operand(&operand, type_double)) {
		syntax_error(typespec->pos, "Projection type '%s' is not "
				"convertible to double",
				type_name(operand.type->kind));
	}

	if (eval_stmt) {
		assert(interpreter);
		Sym *sym = new_sym_project(name, SYM_DIM, type, operand.val.d);
		buf_push(interpreter->local_syms, sym);
	}
	return 1;
}

static unsigned sym_push_dim(const char *name, Type *type, unsigned eval_stmt)
{
	if (!interpreter->properties.is_init) {
		return 1;
	}
	if (sym_get(name)) {
		return 0;
	}
	if (eval_stmt) {
		Sym *sym = new_sym(name, SYM_DIM, type);
		assert(interpreter);
		if (type_string == type) {
			sym->val.s = str_intern("");
		}
		buf_push(interpreter->local_syms, sym);
	}
	return 1;
}

Operand operand_null;

static Operand operand_lvalue(Type *type)
{
	return (Operand){
		.type = type,
			.is_lvalue = 1,
	};
}

static Operand operand_const(Type *type, Val val)
{
	return (Operand){
		.type = type,
			.is_const = 1,
			.val = val,
	};
}

#define CASE(k, t, f) \
	case k: \
	switch (type->kind) { \
		case TYPE_BOOLEAN: \
				   if (operand->val.t) { \
					   operand->val.b = true; \
				   } else { \
					   operand->val.b = false; \
				   } \
		break; \
		case TYPE_INT: \
			       operand->val.i = operand->val.t; \
		break; \
		case TYPE_DOUBLE: \
				  operand->val.d = operand->val.t; \
		break; \
		case TYPE_STRING: \
				  buf_printf(val, "%"f, operand->val.t); \
		operand->val.s = str_intern(val); \
		break; \
		default: \
			 assert(0); \
		break; \
	} \
	break;


static unsigned is_convertible(Operand *operand, Type *dest)
{
	Type *src = operand->type;
	if (dest == src) {
		return 1;
	} else if (is_arithmetic_type(dest) && is_arithmetic_type(src)) {
		return 1;
	} else if (is_string_type(dest) && is_concatable(src)) {
		return 1;
	} else {
		return 0;
	}
}

static unsigned is_castable(Operand *operand, Type *dest)
{
	if (is_convertible(operand, dest)) {
		return 1;
	} else {
		return 0;
	}
}

static unsigned cast_operand(Operand *operand, Type *type)
{
	if (operand->type != type) {
		if (!is_castable(operand, type)) {
			return 0;
		}
		char *val = 0;
		switch (operand->type->kind) {
			CASE(TYPE_BOOLEAN, b, "d");
			CASE(TYPE_INT, i, "d");
			CASE(TYPE_DOUBLE, d, "f");
			default:
			assert(0);
		}
		if (type_string == type) {
			assert(val);
			buf_free(val);
		} else {
			assert(!val);
		}
	}
	operand->type = type;
	return 1;
}

static unsigned convert_operand(Operand *operand, Type *type)
{
	if (is_convertible(operand, type)) {
		cast_operand(operand, type);
		operand->is_lvalue = 0;
		return 1;
	}
	return 0;
}

#undef CASE

static void promote_operand(Operand *operand)
{
	switch (operand->type->kind) {
		case TYPE_BOOLEAN:
			cast_operand(operand, type_int);
			break;
		default:
			break;
	}
}

static void unify_operands(Operand *left, Operand *right)
{
	if (left->type == type_string) {
		cast_operand(right, type_string);
	} else if (right->type == type_string) {
		cast_operand(left, type_string);
	}else if (left->type == type_double) {
		cast_operand(right, type_double);
	} else if (right->type == type_double) {
		cast_operand(left, type_double);
	} else {
		assert(is_arithmetic_type(left->type));
		assert(is_arithmetic_type(right->type));
		promote_operand(left);
		promote_operand(right);
		assert(is_integer_type(left->type));
		assert(is_integer_type(right->type));
	}
	assert(left->type == right->type);
}

static Sym *resolve_name(SrcPos pos, const char *name)
{
	Sym *sym = sym_get(name);
	if (!sym) {
		syntax_error(pos, "Non-existent name '%s'", name);
		return 0;
	}
	return sym;
}

static Type *resolve_typespec_name(SrcPos pos, const char *name)
{
	Sym *sym = resolve_name(pos, name);
	if (sym->kind != SYM_TYPE) {
		syntax_error(pos, "%s must denote a type", name);
		return 0;
	}
	return sym->type;
}

static Type *resolve_typespec(SrcPos pos, Typespec *typespec)
{
	assert(typespec);
	Type *type = 0;
	if (TYPESPEC_PROJECT == typespec->kind) {
		type = resolve_typespec_name(pos, typespec->base->name);
	} else {
		assert(TYPESPEC_NAME == typespec->kind);
		type = resolve_typespec_name(pos, typespec->name);
	}
	assert(type);
	return type;
}

static long long eval_binary_op_i(TokenKind op, long long left, long long right)
{
	switch (op) {
		case TOKEN_MUL:
			return left * right;
		case TOKEN_DIV:
			return right != 0 ? left / right : 0;
		case TOKEN_MOD:
			return right != 0 ? left % right : 0;
		case TOKEN_AND:
			assert(0);
			break;
		case TOKEN_ADD:
			return left + right;
		case TOKEN_SUB:
			return left - right;
		case TOKEN_ASSIGN:
			return left == right;
		case TOKEN_NOTEQ:
			return left != right;
		case TOKEN_LT:
			return left < right;
		case TOKEN_LTEQ:
			return left <= right;
		case TOKEN_GT:
			return left > right;
		case TOKEN_GTEQ:
			return left >= right;
		default:
			assert(0);
			break;
	}
	return 0;
}

static double eval_binary_op_d(TokenKind op, double left, double right)
{
	switch (op) {
		case TOKEN_MUL:
			return left * right;
		case TOKEN_DIV:
			return right != 0 ? left / right : 0;
		case TOKEN_MOD:
			assert(0);
			break;
		case TOKEN_AND:
			assert(0);
			break;
		case TOKEN_ADD:
			return left + right;
		case TOKEN_SUB:
			return left - right;
		case TOKEN_ASSIGN:
			return left == right;
		case TOKEN_NOTEQ:
			return left != right;
		case TOKEN_LT:
			return left < right;
		case TOKEN_LTEQ:
			return left <= right;
		case TOKEN_GT:
			return left > right;
		case TOKEN_GTEQ:
			return left >= right;
		default:
			assert(0);
			break;
	}
	return 0;
}

#define RETURN(t) \
	switch (op) { \
		case TOKEN_ADD: \
				operand.val.t = +operand.val.t; \
		break; \
		case TOKEN_SUB: \
				operand.val.t = -operand.val.t; \
		break; \
		default: \
			 assert(0); \
		break; \
	} \
	return operand.val;

static Val eval_unary_op(TokenKind op, Type *type, Val val)
{
	Operand operand = operand_const(type, val);
	if (is_integer_type(type)) {
		RETURN(i);
	} else if (is_floating_type(type)) {
		RETURN(d);
	} else {
		assert(0);
		return (Val){0};
	}
}

#undef RETURN

#define RETURN_RESULT(T, t) \
	Operand left_operand = operand_const(type, left); \
	Operand right_operand = operand_const(type, right); \
	Operand result_operand; \
	result_operand = operand_const(T, \
			(Val){.t = eval_binary_op_##t(op, left_operand.val.t, \
					right_operand.val.t)}); \
					return result_operand.val;
static Val eval_binary_op(TokenKind op, Type *type, Val left, Val right)
{
	if (is_integer_type(type)) {
		RETURN_RESULT(type_int, i);
	} else if (is_floating_type(type)) {
		RETURN_RESULT(type_double, d);
	} else {
		assert(0);
		return (Val){0};
	}
}
#undef RETURN_RESULT

static Operand resolve_expected_expr(Expr *expr, Type *expected_type);
static Operand resolve_expr(Expr *expr)
{
	return resolve_expected_expr(expr, 0);
}

static Operand resolve_name_operand(SrcPos pos, const char *name)
{
	Sym *sym = sym_get(name);
	if (!sym) {
		syntax_error(pos, "Undeclared variable '%s'", name);
	}
	if (sym->kind == SYM_DIM) {
		Operand operand = operand_lvalue(sym->type);
		operand.val = sym->val;
		return operand;
	} else if (sym->kind == SYM_FUNC) {
		Operand operand = operand_lvalue(sym->type);
		return operand;
	} else {
		syntax_error(pos, "%s must be a declared variable", name);
		return operand_null;
	}
}

static Operand resolve_expr_name(Expr *expr)
{
	assert(expr->kind == EXPR_NAME);
	return resolve_name_operand(expr->pos, expr->name);
}

static Operand resolve_unary_op(TokenKind op, Operand operand)
{
	if (operand.is_const) {
		return operand_const(operand.type,
				eval_unary_op(op, operand.type, operand.val));
	} else {
		return operand;
	}
}

static Operand resolve_expr_unary(Expr *expr)
{
	Operand operand = resolve_expr(expr->unary.expr);
	Type *type = operand.type;
	switch (expr->unary.op) {
		case TOKEN_ADD:
		case TOKEN_SUB:
			if (!is_scalar_type(type)) {
				syntax_error(expr->pos, "Can only use unary "
						"'%s' with scalar types",
						token_kind_name(
							expr->unary.op));
			}
			return resolve_unary_op(expr->unary.op, operand);
		default:
			assert(0);
			break;
	}
	return (Operand){0};
}

static Operand resolve_binary_op(TokenKind op, Operand left, Operand right)
{
	return operand_const(left.type, eval_binary_op(op, left.type, left.val,
				right.val));
}

static Operand resolve_binary_arithmetic_op(TokenKind op, Operand left,
		Operand right)
{
	if (TOKEN_DIV == op)
		cast_operand(&left, type_double);
	unify_operands(&left, &right);
	return resolve_binary_op(op, left, right);
}

static Operand concat_operands(Operand left, Operand right)
{
	assert(type_string == left.type);
	assert(type_string == right.type);
	char *val = 0;
	buf_printf(val, "%s%s", left.val.s, right.val.s);
	Operand operand = operand_const(type_string,
			(Val){.s = str_intern(val)});
	buf_free(val);
	return operand;
}

#define CONVERT(operand) \
	if (!convert_operand(&operand, type_string)) { \
		syntax_error(pos, "Type mismatch, expected '%s', got '%s'", \
				type_name(TYPE_STRING), \
				type_name(operand.type->kind)); \
	}

static Operand resolve_binary_string_op(SrcPos pos, TokenKind op, Operand left,
		Operand right)
{
	if (!(is_concatable(left.type) && is_concatable(right.type))) {
		syntax_error(pos, "Both operands of '%s' must have "
				"string or scalar types", token_kind_name(op));
	}
	CONVERT(left);
	CONVERT(right);
	switch (op) {
		case TOKEN_AND:
		case TOKEN_ADD:
			return concat_operands(left, right);
		case TOKEN_MUL:
		case TOKEN_DIV:
		case TOKEN_LT:
		case TOKEN_LTEQ:
		case TOKEN_GT:
		case TOKEN_GTEQ:
		case TOKEN_SUB:
		case TOKEN_MOD:
			syntax_error(pos, "Both operands of '%s' must have "
					"scalar types", token_kind_name(op));
			break;
		case TOKEN_ASSIGN:
			return operand_const(type_boolean,
					(Val){.b = left.val.s == right.val.s});
		case TOKEN_NOTEQ:
			return operand_const(type_boolean,
					(Val){.b = left.val.s != right.val.s});
		default:
			assert(0);
			break;
	}
	assert(0);
	return (Operand){0};
}

#undef CONVERT

static Operand resolve_expr_binary_op(TokenKind op, const char *op_name,
		SrcPos pos, Operand left, Operand right)
{
	if (is_string_type(left.type) || is_string_type(right.type)) {
		return resolve_binary_string_op(pos, op, left, right);
	}
	assert(is_arithmetic_type(left.type));
	assert(is_arithmetic_type(right.type));
	Operand result = (Operand){0};

	switch (op) {
		case TOKEN_ADD:
		case TOKEN_MUL:
		case TOKEN_DIV:
		case TOKEN_SUB:
		case TOKEN_MOD:
			return resolve_binary_arithmetic_op(op, left, right);
		case TOKEN_ASSIGN:
		case TOKEN_NOTEQ:
		case TOKEN_LT:
		case TOKEN_LTEQ:
		case TOKEN_GT:
		case TOKEN_GTEQ:
			result = resolve_binary_arithmetic_op(op, left, right);
			cast_operand(&result, type_boolean);
			return result;
		case TOKEN_AND_AND:
		case TOKEN_OR_OR:
			cast_operand(&left, type_boolean);
			cast_operand(&right, type_boolean);
			bool b = false;
			if (op == TOKEN_AND_AND) {
				b = left.val.b && right.val.b;
			} else {
				assert(op == TOKEN_OR_OR);
				b = left.val.b || right.val.b;
			}
			return operand_const(type_boolean, (Val){.b = b});
		case TOKEN_AND:
			return resolve_binary_string_op(pos, op, left, right);
		default:
			die("'%s' not implemented yet", op_name);
			break;
	}
	assert(0);
	return (Operand){0};
}

static Operand resolve_expr_call_default(Operand func, Expr *expr, Sym *sym)
{
	double *args = 0;
	size_t num_params = func.type->func.num_params;
	for (size_t i = 0; i < expr->call.num_args; i++) {
		Type *param_type = i < num_params ? func.type->func.params[i]
			: func.type->func.varargs_type;
		Operand arg = resolve_expected_expr(expr->call.args[i],
				param_type);
		if (!convert_operand(&arg, param_type)) {
			syntax_error(expr->call.args[i]->pos, "Invalid type in "
					"function call argument. Expected %s, "
					"got %s", type_name(param_type->kind),
					type_name(arg.type->kind));
		}
		assert(type_double == arg.type);
		buf_push(args, arg.val.d);	
	}
	Val val = (Val){.d = sym->func(args, expr->call.num_args)};
	buf_free(args);
	return operand_const(func.type->func.ret, val);
}

static Operand resolve_expr_call(Expr *expr)
{
	assert(expr->kind == EXPR_CALL);
	Sym *sym = resolve_name(expr->pos, expr->call.expr->name);
	Operand func = resolve_expr(expr->call.expr);
	if (func.type->kind != TYPE_FUNC) {
		syntax_error(expr->pos, "Cannot call non-function value");
	}
	size_t num_params = func.type->func.num_params;
	if (expr->call.num_args < num_params) {
		syntax_error(expr->pos, "Function call with too few arguments");
	}
	if (expr->call.num_args > num_params && !func.type->func.has_varargs) {
		syntax_error(expr->pos, "Function call with too many arguments");
	}
	return resolve_expr_call_default(func, expr, sym);
}

#define INDEX(t1, t2) \
	result = operand_const(type_##t1, (Val){.t2 \
			= record_##t1(db, num_record, title)}); \
			break;

static Operand resolve_expr_index(Expr *index, Type *type)
{
	const Interpreter *i = interpreter;
	if (!i->db) {
		syntax_error(index->pos, "Database not set");
	}
	const Database *db = i->db;
	size_t num_record = i->num_record;
	assert(index);
	assert(type);

	Operand result = (Operand){0};
	Operand op = resolve_expected_expr(index->index.index, type_string);
	const char *title = op.val.s;

	switch (type->kind) {
		case TYPE_NONE:
			assert(0);
			break;
		case TYPE_INT:
			INDEX(int, i);
			break;
		case TYPE_DOUBLE:
			INDEX(double, d);
			break;
		case TYPE_STRING:
			result = operand_const(type_string, (Val){.s
					= str_intern(record_string(db,
								num_record,
								title))});
			break;
		case TYPE_BOOLEAN:
			INDEX(boolean, b);
			break;
		default:
			assert(0);
			break;
	}

	return result;
}

#undef INDEX

static Operand resolve_expr_binary(Expr *expr)
{
	assert(expr->kind == EXPR_BINARY);
	Operand left = resolve_expr(expr->binary.left);
	Operand right = resolve_expr(expr->binary.right);
	TokenKind op = expr->binary.op;
	const char *op_name = token_kind_name(op);
	return resolve_expr_binary_op(op, op_name, expr->pos, left, right);
}

static Operand resolve_expr_boolean(Expr *expr)
{
	assert(expr->kind == EXPR_BOOLEAN);
	bool val = expr->boolean_lit.val;
	Operand operand = operand_const(type_boolean, (Val){.b = val});
	return operand;
}

static Operand resolve_expr_int(Expr *expr)
{
	assert(expr->kind == EXPR_INT);
	unsigned long long val = expr->int_lit.val;
	Operand operand = operand_const(type_int, (Val){.i = val});
	unsigned overflow = val > INT_MAX;
	if (overflow) {
		syntax_error(expr->pos, "Integer literal overflow");
	}
	return operand;
}

static Operand resolve_expr_float(Expr *expr)
{
	assert(expr->kind == EXPR_FLOAT);
	double val = expr->float_lit.val;
	Operand operand = operand_const(type_double, (Val){.d = val});
	return operand;
}

static Operand resolve_expr_string(Expr *expr)
{
	assert(expr->kind == EXPR_STR);
	const char *val = expr->str_lit.val;
	Operand operand = operand_const(type_string,
			(Val){.s = str_intern(val)});
	return operand;
}

static Operand resolve_expected_expr(Expr *expr, Type *expected_type)
{
	switch (expr->kind) {
		case EXPR_PAREN:
			return resolve_expected_expr(expr->paren.expr,
					expected_type);
		case EXPR_BOOLEAN:
			return resolve_expr_boolean(expr);
		case EXPR_INT:
			return resolve_expr_int(expr);
		case EXPR_FLOAT:
			return resolve_expr_float(expr);
		case EXPR_STR:
			return resolve_expr_string(expr);
		case EXPR_NAME:
			return resolve_expr_name(expr);
		case EXPR_CALL:
			return resolve_expr_call(expr);
		case EXPR_INDEX:
			return resolve_expr_index(expr, expected_type);
		case EXPR_UNARY:
			return resolve_expr_unary(expr);
		case EXPR_BINARY:
			return resolve_expr_binary(expr);
		default:
			assert(0);
			return operand_null;
	}
}

static void resolve_stmt(Stmt *stmt, unsigned eval_stmt);

static unsigned is_cond_operand(Operand operand)
{
	return is_arithmetic_type(operand.type);
}

static Operand resolve_cond_expr(Expr *expr)
{
	Operand cond = resolve_expr(expr);
	if (!is_cond_operand(cond)) {
		syntax_error(expr->pos, "Conditional expression must have "
				"arithmetic type");
	}
	if (!cast_operand(&cond, type_boolean)) {
		syntax_error(expr->pos, "Invalid type in condition. "
				"Expected '%s', got '%s'",
				type_name(TYPE_BOOLEAN),
				type_name(cond.type->kind));

	}
	assert(TYPE_BOOLEAN == cond.type->kind);
	return cond;
}

static void resolve_stmt_block(StmtBlock block, unsigned eval_stmt)
{
	for (size_t i = 0; i < block.num_stmts; i++) {
		resolve_stmt(block.stmts[i], eval_stmt);
	}
}

static void resolve_stmt_assign(Stmt *stmt, unsigned eval_stmt)
{
	assert(stmt->kind == STMT_ASSIGN);
	assert(TOKEN_ASSIGN == stmt->assign.op);
	Expr *left_expr = stmt->assign.left;
	Operand left = resolve_expr(left_expr);
	if (!left.is_lvalue) {
		syntax_error(stmt->pos, "Cannot assign to non-lvalue");
	}
	Expr *right_expr = stmt->assign.right;
	Operand right = resolve_expected_expr(right_expr, left.type);
	if (!convert_operand(&right, left.type)) {
		syntax_error(stmt->pos, "Invalid type in assignment. "
				"Expected '%s', got '%s'",
				type_name(left.type->kind),
				type_name(right.type->kind));

	}
	if (eval_stmt) {
		Sym *sym = sym_get(left_expr->name);
		if (sym->is_project) {
			assert(is_floating_type(sym->type));
			assert(is_floating_type(right.type));
			int years = interpreter->project_years;
			double val = sym->project.val;
			sym->val.d = right.val.d * pow(1 + val, years);
		} else {
			sym->val = right.val;
		}
	}
}

static void resolve_stmt_dim(Stmt *stmt, unsigned eval_stmt)
{
	assert(STMT_DIM == stmt->kind);
	const char *name = 0;
	Typespec *typespec = 0;
	Type *type = 0;
	for (size_t i = 0; i < stmt->dim_stmt.num_dims; i++) {
		name = stmt->dim_stmt.dims[i].name;
		typespec = stmt->dim_stmt.dims[i].type;
		type = resolve_typespec(stmt->pos, typespec);
		assert(type);
		if (TYPESPEC_NAME == typespec->kind) {
			if (!sym_push_dim(name, type, eval_stmt)) {
				syntax_error(stmt->pos, "variable '%s' declared"
						" multiple times", name);
			}
		} else if (TYPESPEC_PROJECT == typespec->kind) {
			if (!sym_push_project(name, type, eval_stmt, typespec))
			{
				syntax_error(stmt->pos, "variable '%s' declared"
						" multiple times", name);
			}

		} else {
			assert(0);
		}
	}
}

static void resolve_stmt_for(Stmt *stmt, unsigned eval_stmt)
{
	Operand cond = (Operand){0};
	Operand step = (Operand){0};
	assert(stmt->for_stmt.dim);
	resolve_stmt(stmt->for_stmt.dim, eval_stmt);
	assert(stmt->for_stmt.cond);
	assert(stmt->for_stmt.next);
	resolve_stmt(stmt->for_stmt.next, false);
	step = resolve_expr(stmt->for_stmt.step);
	if (!convert_operand(&step, type_double)) {
		syntax_error(stmt->pos, "Step value in for loop not a scalar "
				"type");
	}
	if (step.val.d < 0.0) {
		stmt->for_stmt.cond->binary.op = TOKEN_GTEQ;
	}
	cond = resolve_cond_expr(stmt->for_stmt.cond);
	resolve_stmt_block(stmt->for_stmt.block, false);
	while (cond.val.b) {
		resolve_stmt_block(stmt->for_stmt.block,
				eval_stmt);
		resolve_stmt(stmt->for_stmt.next, eval_stmt);
		cond = resolve_cond_expr(stmt->for_stmt.cond);
	}
}

static void resolve_stmt_if(Stmt *stmt)
{
	Operand cond = (Operand){0};
	bool b = false;
	assert(stmt->if_stmt.cond);
	cond = resolve_cond_expr(stmt->if_stmt.cond);
	b = cond.val.b;
	resolve_stmt_block(stmt->if_stmt.then_block, b);
	for (size_t i = 0; i < stmt->if_stmt.num_elseifs; i++){
		ElseIf elseif = stmt->if_stmt.elseifs[i];
		cond = resolve_cond_expr(elseif.cond);
		if (!b) {
			b = cond.val.b;
			resolve_stmt_block(elseif.block, b);
		} else {
			resolve_stmt_block(elseif.block,false);
		}
	}
	if (stmt->if_stmt.else_block.stmts) {
		if (!b) {
			resolve_stmt_block(stmt->if_stmt.
					else_block, true);
		} else {
			resolve_stmt_block(stmt->if_stmt.
					else_block, false);
		}
	}

}

#define PATTERN(i, op, expr) \
	TokenKind op##i = op; \
	const char *op_name##i = token_kind_name(op##i); \
	Operand operand##i = resolve_expr(expr); \
	Operand result##i = (Operand){0}; \
	result##i = resolve_expr_binary_op(op##i, op_name##i, start->pos, \
			operand, operand##i); \
			assert(TYPE_BOOLEAN == result##i.type->kind);


static bool resolve_to_pattern(Operand operand, SelectCasePattern pattern)
{
	assert(PATTERN_TO == pattern.kind);
	Expr *start = pattern.to_pattern.start;
	Expr *end = pattern.to_pattern.end;
	PATTERN(1, TOKEN_GTEQ, start);
	PATTERN(2, TOKEN_LTEQ, end);

	return result1.val.b && result2.val.b;
}

#undef PATTERN

#define PATTERN(e, o) \
	expr = e; \
	pat = resolve_expr(expr); \
	op = o; \
	op_name = token_kind_name(op); \
	result = resolve_expr_binary_op(op, op_name, expr->pos, operand, pat); \
	assert(TYPE_BOOLEAN == result.type->kind); \
	b = result.val.b;

static bool resolve_select_case_pattern(Operand operand,
		SelectCasePattern pattern)
{
	Expr *expr = 0;
	Operand pat = (Operand){0};
	Operand result = (Operand){0};
	TokenKind op = TOKEN_EOF;
	bool b = false;
	const char *op_name = 0;
	switch (pattern.kind) {
		case PATTERN_LIT:
			PATTERN(pattern.expr, TOKEN_ASSIGN);
			break;
		case PATTERN_TO:
			b = resolve_to_pattern(operand, pattern);
			break;
		case PATTERN_IS:
			PATTERN(pattern.is_pattern.expr,pattern.is_pattern.op);
			break;
		default:
			assert(0);
			break;
	}
	if (PATTERN_TO != pattern.kind) {
		assert(expr);
	}
	return b;
}

#undef PATTERN

static bool resolve_select_case(Operand operand, SelectCase sc)
{
	resolve_stmt_block(sc.block, false);
	bool b = false;
	if (sc.is_default) {
		b = true;
	} else {
		for (size_t i = 0; i < sc.num_patterns; i++) {
			b = b || resolve_select_case_pattern(operand,
					sc.patterns[i]);	
		}
	}
	return b;
}

static void resolve_stmt_select_case(Stmt *stmt, unsigned eval_stmt)
{
	assert(STMT_SELECT_CASE == stmt->kind);
	assert(stmt->select_case_stmt.expr);
	Operand operand = resolve_expr(stmt->select_case_stmt.expr);	
	bool b = false;
	size_t sc = 0;
	for (size_t i = 0; i < stmt->select_case_stmt.num_cases; i++) {
		if (!b) {
			b = resolve_select_case(operand,
					stmt->select_case_stmt.cases[i]);
			sc = i;
		} else {
			resolve_select_case(operand,
					stmt->select_case_stmt.cases[i]);
		}
	}

	if (b) {
		resolve_stmt_block(stmt->select_case_stmt.cases[sc].block,
				eval_stmt);
	}
}

#define LOOP(loop, b) \
	cond = resolve_cond_expr(stmt->loop##_stmt.cond); \
	resolve_stmt_block(stmt->loop##_stmt.block, false); \
	while (b) { \
		resolve_stmt_block(stmt->loop##_stmt.block, true); \
		cond = resolve_cond_expr(stmt->loop##_stmt.cond); \
	}
#define DO_LOOP(loop, b) \
	cond = resolve_cond_expr(stmt->loop##_stmt.cond); \
	resolve_stmt_block(stmt->loop##_stmt.block, false); \
	do { \
		resolve_stmt_block(stmt->loop##_stmt.block, true); \
		cond = resolve_cond_expr(stmt->loop##_stmt.cond); \
	} while (b);

static void resolve_stmt(Stmt *stmt, unsigned eval_stmt)
{
	Operand cond = (Operand){0};
	switch (stmt->kind) {
		case STMT_BLOCK:
			resolve_stmt_block(stmt->block, eval_stmt);
			break;
		case STMT_IF:
			resolve_stmt_if(stmt);
			break;
		case STMT_WHILE:
		case STMT_DO_WHILE_LOOP:
			LOOP(while, cond.val.b);
			break;
		case STMT_DO_WHILE:
			DO_LOOP(while, cond.val.b);
			break;
		case STMT_DO_UNTIL:
			DO_LOOP(until, !cond.val.b);
			break;
		case STMT_DO_UNTIL_LOOP:
			LOOP(until, !cond.val.b);
			break;
		case STMT_FOR:
			resolve_stmt_for(stmt, eval_stmt);
			break;
		case STMT_SELECT_CASE:
			resolve_stmt_select_case(stmt, eval_stmt);
			break;
		case STMT_ASSIGN:
			resolve_stmt_assign(stmt, eval_stmt);
			break;
		case STMT_DIM:
			resolve_stmt_dim(stmt, eval_stmt);
			break;
		case STMT_EXPR:
			resolve_expr(stmt->expr);
			break;
		default:
			assert(0);
			break;
	}
}

#undef LOOP
#undef DO_LOOP

void resolve_stmts(Stmt **stmts)
{
	for (size_t i = 0; i < buf_len(stmts); i++) {
		resolve_stmt(stmts[i], 1);
	}
}

