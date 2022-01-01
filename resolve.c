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
			printf("%s: TODO\n", sym->name);
			break;
		case TYPE_INT:
			printf("%s: %d\n", sym->name, sym->val.i);
			break;
		case TYPE_DOUBLE:
			printf("%s: TODO\n", sym->name);
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

/* TODO */
unsigned cast_operand(Operand *operand, Type *type)
{
	return 1;
}

void unify_arithmetic_operands(Operand *left, Operand *right)
{
    if (left->type == type_double) {
        cast_operand(right, type_double);
    } else if (right->type == type_double) {
        cast_operand(left, type_double);
    } else {
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
        return left & right;
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
	    return (Val){0};
    }
}

Val eval_binary_op(TokenKind op, Type *type, Val left, Val right)
{
	if (is_integer_type(type)) {
		Operand left_operand = operand_const(type, left);
		Operand right_operand = operand_const(type, right);
		Operand result_operand;
		result_operand = operand_const(type_int, (Val){.i = eval_binary_op_i(op, left_operand.val.i, right_operand.val.i)});
		return result_operand.val;
	} else {
		return (Val){0};
	}
}

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
	if (left.is_const && right.is_const) {
		return operand_const(left.type, eval_binary_op(op, left.type, left.val, right.val));
	} else {
		return operand_rvalue(left.type);
	}
}

Operand resolve_binary_arithmetic_op(TokenKind op, Operand left, Operand right)
{
	unify_arithmetic_operands(&left, &right);
	return resolve_binary_op(op, left, right);
}

Operand resolve_expr_binary_op(TokenKind op, const char *op_name, SrcPos pos, Operand left, Operand right, Expr *left_expr, Expr *right_expr)
{
	switch (op) {
		case TOKEN_MUL:
		case TOKEN_DIV:
			if (!is_arithmetic_type(left.type)) {
				fatal_error(pos, "Left operand of %s must have arithmetic type", op_name);
			}
			if (!is_arithmetic_type(right.type)) {
				fatal_error(pos, "Right operand of %s must have arithmetic type", op_name);
			}
			return resolve_binary_arithmetic_op(op, left, right);
		case TOKEN_MOD:
			if (!is_integer_type(left.type)) {
				fatal_error(pos, "Left operand of %% must have integer type");
			}
			if (!is_integer_type(right.type)) {
				fatal_error(pos, "Right operand of %% must have integer type");
			}
			return resolve_binary_arithmetic_op(op, left, right);
		case TOKEN_ADD:
			if (is_arithmetic_type(left.type) && is_arithmetic_type(right.type)) {
				return resolve_binary_arithmetic_op(op, left, right);
			} else {
				fatal_error(pos, "Operands of + must both have arithmetic type, or pointer and integer type");
			}
			break;
		case TOKEN_SUB:
			if (is_arithmetic_type(left.type) && is_arithmetic_type(right.type)) {
				return resolve_binary_arithmetic_op(op, left, right);
			} else {
				fatal_error(pos, "Operands of - must both have arithmetic type, pointer and integer type, or compatible pointer types");
			}
			break;
		case TOKEN_ASSIGN:
		case TOKEN_NOTEQ:
			if (is_arithmetic_type(left.type) && is_arithmetic_type(right.type)) {
				Operand result = resolve_binary_arithmetic_op(op, left, right);
				cast_operand(&result, type_int);
				return result;
			} else {
				fatal_error(pos, "Operands of %s must be arithmetic types or compatible pointer types", op_name);
			}
			break;
		case TOKEN_LT:
		case TOKEN_LTEQ:
		case TOKEN_GT:
		case TOKEN_GTEQ:
			if (is_arithmetic_type(left.type) && is_arithmetic_type(right.type)) {
				Operand result = resolve_binary_arithmetic_op(op, left, right);
				cast_operand(&result, type_int);
				return result;
			} else {
				fatal_error(pos, "Operands of %s must be arithmetic types or compatible pointer types", op_name);
			}
			break;
		case TOKEN_AND_AND:
		case TOKEN_OR_OR:
			if (is_scalar_type(left.type) && is_scalar_type(right.type)) {
				if (left.is_const && right.is_const) {
					cast_operand(&left, type_boolean);
					cast_operand(&right, type_boolean);
					int i;
					if (op == TOKEN_AND_AND) {
						i = left.val.b && right.val.b;
					} else {
						assert(op == TOKEN_OR_OR);
						i = left.val.b || right.val.b;
					}
					return operand_const(type_int, (Val){.i = i});
				} else {
					return operand_rvalue(type_int);
				}
			} else {
				fatal_error(pos, "Operands of %s must have scalar types", op_name);
			}
			break;
		default:
			assert(0);
			break;
	}
	return (Operand){0};
}

Operand resolve_expr_binary(Expr *expr)
{
	assert(expr->kind == EXPR_BINARY);
	Operand left = resolve_expr(expr->binary.left);
	Operand right = resolve_expr(expr->binary.right);
	TokenKind op = expr->binary.op;
	const char *op_name = token_kind_name(op);
	return resolve_expr_binary_op(op, op_name, expr->pos, left, right, expr->binary.left, expr->binary.right);
}

Operand resolve_expr_int(Expr *expr)
{
	assert(expr->kind == EXPR_INT);
	unsigned long long int_max = type_metrics[TYPE_INT].max;
	unsigned long long val = expr->int_lit.val;
	Operand operand = operand_const(type_int, (Val){.i = val});
	unsigned overflow = val > int_max;
	if (overflow) {
		fatal_error(expr->pos, "Integer literal overflow");
	}
	return operand;
}

Operand resolve_expected_expr(Expr *expr, Type *expected_type)
{
	switch (expr->kind) {
		case EXPR_INT:
			return resolve_expr_int(expr);
		case EXPR_FLOAT:
			return operand_const(type_double, (Val){0});
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
	if (right.type != left.type) {
		fatal_error(stmt->pos, "Invalid type in assignment. "
				"Expected %s, got %s",
				type_names[left.type->kind],
				type_names[right.type->kind]);

	}
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
