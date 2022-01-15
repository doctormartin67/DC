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
		default:
			assert(0);
			break;
	}
}

void print_syms(void)
{
	printf("all symbols:\n");
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

unsigned sym_push_dim(const char *name, Type *type)
{
	if (sym_get(name)) {
		return 0;
	}
	if (syms_end == syms + MAX_SYMS) {
		fatal("Too many symbols");
	}
	*syms_end++ = (Sym){
		.name = str_intern(name),
			.kind = SYM_DIM,
			.type = type,
	};
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

#define CASE(k, t) \
	case k: \
	switch (type->kind) { \
		case TYPE_BOOLEAN: \
				   if (operand->val.t) { \
					   operand->val.b = TRUE; \
				   } else { \
					   operand->val.b = FALSE; \
				   } \
		break; \
		case TYPE_INT: \
			       operand->val.i = operand->val.t; \
		break; \
		case TYPE_DOUBLE: \
				  operand->val.d = operand->val.t; \
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
		switch (operand->type->kind) {
			CASE(TYPE_BOOLEAN, b);
			CASE(TYPE_INT, i);
			CASE(TYPE_DOUBLE, d);
			default:
			assert(0);
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
void unify_arithmetic_operands(Operand *left, Operand *right)
{
	if (left->type == type_double) {
		cast_operand(right, type_double);
	} else if (right->type == type_double) {
		cast_operand(left, type_double);
	} else {
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

Val eval_unary_op(TokenKind op, Type *type, Val val)
{
	if (is_integer_type(type)) {
		Operand operand = operand_const(type, val);
		switch (op) {
			case TOKEN_ADD:
				operand.val.i = +operand.val.i;
				break;
			case TOKEN_SUB:
				operand.val.i = -operand.val.i;
				break;
			default:
				assert(0);
				break;
		}
		return operand.val;
	} else {
		assert(0);
		return (Val){0};
	}
}

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
			if (!is_arithmetic_type(type)) {
				fatal_error(expr->pos, "Can only use unary %s"
						"with arithmetic types",
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
	unify_arithmetic_operands(&left, &right);
	return resolve_binary_op(op, left, right);
}

#define FATAL_ERROR(O, T) fatal_error(pos, O" operand of '%s' must have " \
		T" type", op_name);
Operand resolve_expr_binary_op(TokenKind op, const char *op_name, SrcPos pos,
		Operand left, Operand right, Expr *left_expr, Expr *right_expr)
{
	switch (op) {
		case TOKEN_MUL:
		case TOKEN_DIV:
		case TOKEN_ADD:
		case TOKEN_SUB:
			if (!is_arithmetic_type(left.type)) {
				FATAL_ERROR("Left", "arithmetic");
			}
			if (!is_arithmetic_type(right.type)) {
				FATAL_ERROR("Right", "arithmetic");
			}
			return resolve_binary_arithmetic_op(op, left, right);
		case TOKEN_MOD:
			if (!is_integer_type(left.type)) {
				FATAL_ERROR("Left", "integer");
			}
			if (!is_integer_type(right.type)) {
				FATAL_ERROR("Reft", "integer");
			}
			return resolve_binary_arithmetic_op(op, left, right);
		case TOKEN_ASSIGN:
		case TOKEN_NOTEQ:
			if (is_arithmetic_type(left.type)
					&& is_arithmetic_type(right.type)) {
				Operand result = resolve_binary_arithmetic_op(
						op, left, right);
				cast_operand(&result, type_boolean);
				return result;
			} else {
				fatal_error(pos, "Operands of '%s' must be "
						"arithmetic types", op_name);
			}
			break;
		case TOKEN_LT:
		case TOKEN_LTEQ:
		case TOKEN_GT:
		case TOKEN_GTEQ:
			if (is_scalar_type(left.type)
					&& is_scalar_type(right.type)) {
				Operand result = resolve_binary_arithmetic_op(
						op, left, right);
				cast_operand(&result, type_boolean);
			} else {
				fatal_error(pos, "Operands of '%s' must have "
						"scalar types", op_name);
			}
			break;
		case TOKEN_AND_AND:
		case TOKEN_OR_OR:
			if (is_boolean_type(left.type)
					&& is_boolean_type(right.type)) {
				cast_operand(&left, type_boolean);
				cast_operand(&right, type_boolean);
				boolean b = FALSE;
				if (op == TOKEN_AND_AND) {
					b = left.val.b && right.val.b;
				} else {
					assert(op == TOKEN_OR_OR);
					b = left.val.b || right.val.b;
				}
				return operand_const(type_boolean,
						(Val){.b = b});
			} else {
				fatal_error(pos, "Operands of '%s' must have "
						"boolean types", op_name);
			}
			break;
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
	return resolve_expr_binary_op(op, op_name, expr->pos, left, right,
			expr->binary.left, expr->binary.right);
}

Operand resolve_expr_boolean(Expr *expr)
{
	assert(expr->kind == EXPR_BOOLEAN);
	boolean val = expr->boolean_lit.val;
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
#if 0
		case EXPR_STR:
			return resolved_rvalue(type_ptr(type_char));
#endif
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

void resolve_stmt_assign(Stmt *stmt)
{
	assert(stmt->kind == STMT_ASSIGN);
	assert(TOKEN_ASSIGN == stmt->assign.op);
	Expr *left_expr = stmt->assign.left;
	Operand left = resolve_expr(left_expr);
	if (!left.is_lvalue) {
		fatal_error(stmt->pos, "Cannot assign to non-lvalue");
	}
	Expr *right_expr = stmt->assign.right;
	Operand right = resolve_expected_expr(right_expr, left.type);
	if (!convert_operand(&right, left.type)) {
		fatal_error(stmt->pos, "Invalid type in assignment. "
				"Expected %s, got %s",
				type_names[left.type->kind],
				type_names[right.type->kind]);

	}
	sym_get(left_expr->name)->val = right.val;
}

void resolve_stmt_dim(Stmt *stmt)
{
	assert(STMT_DIM == stmt->kind);
	Type *type = 0;
	for (size_t i = 0; i < stmt->dim_stmt.num_dims; i++) {
		type = resolve_typespec(stmt->dim_stmt.dims[i].type);
		assert(type);
		if (!sym_push_dim(stmt->dim_stmt.dims[i].name, type)) {
			fatal_error(stmt->pos, "Variable %s declared multiple"
					" times", stmt->dim_stmt.dims[i].name);
		}
	}
}

unsigned resolve_stmt(Stmt *stmt, Type *ret_type)
{
	switch (stmt->kind) {
#if 0
		case STMT_BLOCK:
			return resolve_stmt_block(stmt->block, ret_type, ctx);
		case STMT_IF: {
				      Sym *scope = sym_enter();
				      if (stmt->if_stmt.init) {
					      resolve_stmt_init(stmt->if_stmt.init);
				      }
				      if (stmt->if_stmt.cond) {
					      resolve_cond_expr(stmt->if_stmt.cond);
				      } else if (!is_cond_operand(resolve_name_operand(stmt->pos, stmt->if_stmt.init->init.name))) {
					      fatal_error(stmt->pos, "Conditional expression must have scalar type");
				      }
				      bool returns = resolve_stmt_block(stmt->if_stmt.then_block, ret_type, ctx);
				      for (size_t i = 0; i < stmt->if_stmt.num_elseifs; i++) {
					      ElseIf elseif = stmt->if_stmt.elseifs[i];
					      resolve_cond_expr(elseif.cond);
					      returns = resolve_stmt_block(elseif.block, ret_type, ctx) && returns;
				      }
				      if (stmt->if_stmt.else_block.stmts) {
					      returns = resolve_stmt_block(stmt->if_stmt.else_block, ret_type, ctx) && returns;
				      } else {
					      returns = false;
				      }
				      sym_leave(scope);
				      return returns;
			      }
		case STMT_WHILE:
		case STMT_DO_WHILE:
			      resolve_cond_expr(stmt->while_stmt.cond);
			      ctx.is_break_legal = true;
			      ctx.is_continue_legal = true;
			      resolve_stmt_block(stmt->while_stmt.block, ret_type, ctx);
			      return false;
		case STMT_FOR: {
				       Sym *scope = sym_enter();
				       if (stmt->for_stmt.init) {
					       resolve_stmt(stmt->for_stmt.init, ret_type, ctx);
				       }
				       if (stmt->for_stmt.cond) {
					       resolve_cond_expr(stmt->for_stmt.cond);
				       }
				       if (stmt->for_stmt.next) {
					       resolve_stmt(stmt->for_stmt.next, ret_type, ctx);
				       }
				       ctx.is_break_legal = true;
				       ctx.is_continue_legal = true;
				       resolve_stmt_block(stmt->for_stmt.block, ret_type, ctx);
				       sym_leave(scope);
				       return false;
			       }
		case STMT_SELECT_CASE: {
					       Operand operand = resolve_expr_rvalue(stmt->switch_stmt.expr);
					       if (!is_integer_type(operand.type)) {
						       fatal_error(stmt->pos, "Switch expression must have integer type");
					       }
					       ctx.is_break_legal = true;
					       bool returns = true;
					       bool has_default = false;
					       for (size_t i = 0; i < stmt->switch_stmt.num_cases; i++) {
						       SwitchCase switch_case = stmt->switch_stmt.cases[i];
						       for (size_t j = 0; j < switch_case.num_patterns; j++) {
							       SwitchCasePattern pattern = switch_case.patterns[j];
							       Expr *start_expr = pattern.start;
							       Operand start_operand = resolve_const_expr(start_expr);
							       if (!convert_operand(&start_operand, operand.type)) {
								       fatal_error(start_expr->pos, "Invalid type in switch case expression. Expected %s, got %s", get_type_name(operand.type), get_type_name(start_operand.type));
							       }
							       Expr *end_expr = pattern.end;
							       if (end_expr) {
								       Operand end_operand = resolve_const_expr(end_expr);
								       if (!convert_operand(&end_operand, operand.type)) {
									       fatal_error(end_expr->pos, "Invalid type in switch case expression. Expected %s, got %s", get_type_name(operand.type), get_type_name(end_operand.type));
								       }
								       convert_operand(&start_operand, type_llong);
								       set_resolved_val(start_expr, start_operand.val);
								       convert_operand(&end_operand, type_llong);
								       set_resolved_val(end_expr, end_operand.val);
								       if (end_operand.val.ll < start_operand.val.ll) {
									       fatal_error(start_expr->pos, "Case range end value cannot be less thn start value");
								       }
								       if (end_operand.val.ll - start_operand.val.ll >= 256) {
									       fatal_error(start_expr->pos, "Case range cannot span more than 256 values");
								       }
							       }
						       }
						       if (switch_case.is_default) {
							       if (has_default) {
								       fatal_error(stmt->pos, "Switch statement has multiple default clauses");
							       }
							       has_default = true;
						       }
						       if (switch_case.block.num_stmts > 1) {
							       Stmt *last_stmt = switch_case.block.stmts[switch_case.block.num_stmts - 1];
							       if (last_stmt->kind == STMT_BREAK) {
								       warning(last_stmt->pos, "Case blocks already end with an implicit break");
							       }
						       }
						       returns = resolve_stmt_block(switch_case.block, ret_type, ctx) && returns;
					       }
					       return returns && has_default;
				       }
#endif
		case STMT_ASSIGN:
				       resolve_stmt_assign(stmt);
				       return 0;
		case STMT_DIM:
				       resolve_stmt_dim(stmt);
				       return 0;
		case STMT_EXPR:
				       resolve_expr(stmt->expr);
				       return 0;
		default:
				       assert(0);
	}
}

void resolve_stmts(Stmt **stmts)
{
	for (size_t i = 0; i < buf_len(stmts); i++) {
		resolve_stmt(stmts[i], 0);
	}
}
