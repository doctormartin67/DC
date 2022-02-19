typedef enum SymKind {
	SYM_NONE,
	SYM_DIM,
	SYM_FUNC,
	SYM_TYPE,
} SymKind;

typedef struct Sym {
	const char *name;
	SymKind kind;
	Decl *decl;
	struct {
		Type *type;
		Val val;
	};
} Sym;

enum {
	MAX_SYMS = 1024
};

Sym syms[MAX_SYMS];
Sym *syms_end = syms;

void print_sym_dim(Sym *sym)
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

void print_syms(void)
{
	printf("\nall symbols:\n");
	printf("-------\n");
	for (Sym *sym = syms; sym < syms_end; sym++) {
		switch (sym->kind) {
			case SYM_TYPE:
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
	printf("-------\n");
}

Sym *sym_new(SymKind kind, const char *name, Decl *decl)
{
	Sym *sym = calloc(1, sizeof(*sym));
	sym->kind = kind;
	sym->name = name;
	sym->decl = decl;
	return sym;
}

Sym *sym_decl(Decl *decl)
{
	SymKind kind = SYM_NONE;
	switch (decl->kind) {
		case DECL_DIM:
			kind = SYM_DIM;
			break;
		default:
			assert(0);
			break;
	}
	Sym *sym = sym_new(kind, decl->name, decl);
	return sym;
}

Sym *sym_get(const char *name)
{
	for (Sym *it = syms_end; it != syms; it--) {
		Sym *sym = it - 1;
		if (sym->name == name) {
			return sym;
		}
	}
	return 0;
}

unsigned sym_push_type(const char *name, Type *type)
{
	if (sym_get(name)) {
		return 0;
	}
	if (syms_end == syms + MAX_SYMS) {
		fatal("Too many symbols");
	}
	*syms_end++ = (Sym){
		.name = str_intern(name),
			.kind = SYM_TYPE,
			.type = type,
	};
	return 1;
}

unsigned sym_push_dim(const char *name, Type *type, unsigned eval_stmt)
{
	if (sym_get(name)) {
		return 0;
	}
	if (syms_end == syms + MAX_SYMS) {
		fatal("Too many symbols");
	}
	if (eval_stmt) {
		*syms_end++ = (Sym){
			.name = str_intern(name),
				.kind = SYM_DIM,
				.type = type,
		};
		if (type_string == type) {
			syms_end[-1].val.s = str_intern("");
		}
	}
	return 1;
}

typedef struct Operand {
	Type *type;
	unsigned is_lvalue;
	unsigned is_const;
	Val val;
} Operand;

Operand operand_null;

Operand operand_rvalue(Type *type)
{
	return (Operand){
		.type = type,
	};
}

Operand operand_lvalue(Type *type)
{
	return (Operand){
		.type = type,
			.is_lvalue = 1,
	};
}
Operand operand_const(Type *type, Val val)
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


unsigned is_convertible(Operand *operand, Type *dest)
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

unsigned is_castable(Operand *operand, Type *dest)
{
	if (is_convertible(operand, dest)) {
		return 1;
	} else {
		return 0;
	}
}

unsigned cast_operand(Operand *operand, Type *type)
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

unsigned convert_operand(Operand *operand, Type *type)
{
	if (is_convertible(operand, type)) {
		cast_operand(operand, type);
		operand->is_lvalue = 0;
		return 1;
	}
	return 0;
}

#undef CASE

void promote_operand(Operand *operand)
{
	switch (operand->type->kind) {
		case TYPE_BOOLEAN:
			cast_operand(operand, type_int);
			break;
		default:
			break;
	}
}
void unify_operands(Operand *left, Operand *right)
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

Sym *resolve_name(const char *name)
{
	Sym *sym = sym_get(name);
	if (!sym) {
		fatal("Non-existent name '%s'", name);
		return 0;
	}
	return sym;
}

Type *resolve_typespec(Typespec *typespec)
{
	assert(typespec);
	assert(TYPESPEC_NAME == typespec->kind);
	Sym *sym = resolve_name(typespec->name);
	if (sym->kind != SYM_TYPE) {
		fatal("%s must denote a type", typespec->name);
		return 0;
	}
	return sym->type;
}

long long eval_binary_op_i(TokenKind op, long long left, long long right)
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

double eval_binary_op_d(TokenKind op, double left, double right)
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

Val eval_unary_op(TokenKind op, Type *type, Val val)
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
Val eval_binary_op(TokenKind op, Type *type, Val left, Val right)
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

Operand resolve_expected_expr(Expr *expr, Type *expected_type);
Operand resolve_expr(Expr *expr)
{
	return resolve_expected_expr(expr, 0);
}

Operand resolve_name_operand(SrcPos pos, const char *name)
{
	Sym *sym = sym_get(name);
	if (!sym) {
		fatal_error(pos, "Undeclared variable '%s'", name);
	}
	if (sym->kind == SYM_DIM) {
		Operand operand = operand_lvalue(sym->type);
		operand.val = sym->val;
		return operand;
	} else {
		fatal_error(pos, "%s must be a declared variable", name);
		return operand_null;
	}
}

Operand resolve_expr_name(Expr *expr)
{
	assert(expr->kind == EXPR_NAME);
	return resolve_name_operand(expr->pos, expr->name);
}

Operand resolve_unary_op(TokenKind op, Operand operand)
{
	if (operand.is_const) {
		return operand_const(operand.type,
				eval_unary_op(op, operand.type, operand.val));
	} else {
		return operand;
	}
}

Operand resolve_expr_unary(Expr *expr)
{
	Operand operand = resolve_expr(expr->unary.expr);
	Type *type = operand.type;
	switch (expr->unary.op) {
		case TOKEN_ADD:
		case TOKEN_SUB:
			if (!is_scalar_type(type)) {
				fatal_error(expr->pos, "Can only use unary "
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

Operand resolve_binary_op(TokenKind op, Operand left, Operand right)
{
	return operand_const(left.type, eval_binary_op(op, left.type, left.val,
				right.val));
}

Operand resolve_binary_arithmetic_op(TokenKind op, Operand left, Operand right)
{
	if (TOKEN_DIV == op)
		cast_operand(&left, type_double);
	unify_operands(&left, &right);
	return resolve_binary_op(op, left, right);
}

Operand concat_operands(Operand left, Operand right)
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
		fatal_error(pos, "Type mismatch, expected '%s', got '%s'", \
				type_names[TYPE_STRING], \
				type_names[operand.type->kind]); \
	}

Operand resolve_binary_string_op(SrcPos pos, TokenKind op, Operand left,
		Operand right)
{
	if (!(is_concatable(left.type) && is_concatable(right.type))) {
		fatal_error(pos, "Both operands of '%s' must have "
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
			assert(0);
			break;
		case TOKEN_SUB:
		case TOKEN_MOD:
			fatal_error(pos, "Both operands of '%s' must have "
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

Operand resolve_expr_binary_op(TokenKind op, const char *op_name, SrcPos pos,
		Operand left, Operand right)
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
			assert(0);
			break;
	}
	assert(0);
	return (Operand){0};
}
#undef FATAL_ERROR

Operand resolve_expr_binary(Expr *expr)
{
	assert(expr->kind == EXPR_BINARY);
	Operand left = resolve_expr(expr->binary.left);
	Operand right = resolve_expr(expr->binary.right);
	TokenKind op = expr->binary.op;
	const char *op_name = token_kind_name(op);
	return resolve_expr_binary_op(op, op_name, expr->pos, left, right);
}

Operand resolve_expr_boolean(Expr *expr)
{
	assert(expr->kind == EXPR_BOOLEAN);
	bool val = expr->boolean_lit.val;
	Operand operand = operand_const(type_boolean, (Val){.b = val});
	return operand;
}

Operand resolve_expr_int(Expr *expr)
{
	assert(expr->kind == EXPR_INT);
	unsigned long long val = expr->int_lit.val;
	Operand operand = operand_const(type_int, (Val){.i = val});
	unsigned overflow = val > INT_MAX;
	if (overflow) {
		fatal_error(expr->pos, "Integer literal overflow");
	}
	return operand;
}

Operand resolve_expr_float(Expr *expr)
{
	assert(expr->kind == EXPR_FLOAT);
	double val = expr->float_lit.val;
	Operand operand = operand_const(type_double, (Val){.d = val});
	return operand;
}

Operand resolve_expr_string(Expr *expr)
{
	assert(expr->kind == EXPR_STR);
	char *val = 0;
	buf_printf(val, expr->str_lit.val);
	Operand operand = operand_const(type_string,
			(Val){.s = str_intern(val)});
	buf_free(val);
	return operand;
}

Operand resolve_expected_expr(Expr *expr, Type *expected_type)
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
#if 0
		case EXPR_CALL:
			return resolve_expr_call(expr);
		case EXPR_INDEX:
			return resolve_expr_index(expr);
#endif
		case EXPR_UNARY:
			return resolve_expr_unary(expr);
		case EXPR_BINARY:
			return resolve_expr_binary(expr);
		default:
			assert(0);
			return operand_null;
	}
}

Val resolve_const_expr(Expr *expr)
{
	Operand result = resolve_expr(expr);
	if (!result.is_const) {
		fatal("Expected constant expression");
	}
	return result.val;
}

void resolve_stmt(Stmt *stmt, unsigned eval_stmt);

unsigned is_cond_operand(Operand operand)
{
	return is_arithmetic_type(operand.type);
}

Operand resolve_cond_expr(Expr *expr)
{
	Operand cond = resolve_expr(expr);
	if (!is_cond_operand(cond)) {
		fatal_error(expr->pos, "Conditional expression must have "
				"arithmetic type");
	}
	if (!cast_operand(&cond, type_boolean)) {
		fatal_error(expr->pos, "Invalid type in condition. "
				"Expected '%s', got '%s'",
				type_names[TYPE_BOOLEAN],
				type_names[cond.type->kind]);

	}
	assert(TYPE_BOOLEAN == cond.type->kind);
	return cond;
}

void resolve_stmt_block(StmtBlock block, unsigned eval_stmt)
{
	for (size_t i = 0; i < block.num_stmts; i++) {
		resolve_stmt(block.stmts[i], eval_stmt);
	}
}

void resolve_stmt_assign(Stmt *stmt, unsigned eval_stmt)
{
	assert(stmt->kind == STMT_ASSIGN);
	assert(TOKEN_ASSIGN == stmt->assign.op);
	Expr *left_expr = stmt->assign.left;
	Sym *sym = sym_get(left_expr->name);
	Operand left = resolve_expr(left_expr);
	if (!left.is_lvalue) {
		fatal_error(stmt->pos, "Cannot assign to non-lvalue");
	}
	Expr *right_expr = stmt->assign.right;
	Operand right = resolve_expected_expr(right_expr, left.type);
	if (!convert_operand(&right, left.type)) {
		fatal_error(stmt->pos, "Invalid type in assignment. "
				"Expected '%s', got '%s'",
				type_names[left.type->kind],
				type_names[right.type->kind]);

	}
	if (eval_stmt) {
		sym->val = right.val;
	}
}

void resolve_stmt_dim(Stmt *stmt, unsigned eval_stmt)
{
	assert(STMT_DIM == stmt->kind);
	Type *type = 0;
	for (size_t i = 0; i < stmt->dim_stmt.num_dims; i++) {
		type = resolve_typespec(stmt->dim_stmt.dims[i].type);
		assert(type);
		if (!sym_push_dim(stmt->dim_stmt.dims[i].name, type,
					eval_stmt)) {
			fatal_error(stmt->pos, "Variable '%s' declared "
					"multiple times",
					stmt->dim_stmt.dims[i].name);
		}
	}
}
void resolve_stmt_for(Stmt *stmt, unsigned eval_stmt)
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
		fatal_error(stmt->pos, "Step value in for loop not a scalar "
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

void resolve_stmt_if(Stmt *stmt)
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


bool resolve_to_pattern(Operand operand, SelectCasePattern pattern)
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

bool resolve_select_case_pattern(Operand operand,
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

bool resolve_select_case(Operand operand, SelectCase sc)
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

void resolve_stmt_select_case(Stmt *stmt, unsigned eval_stmt)
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

void resolve_stmt(Stmt *stmt, unsigned eval_stmt)
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
