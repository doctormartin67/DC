Decl *parse_decl_opt(void);
Decl *parse_decl(void);
Typespec *parse_type(void);
Stmt *parse_stmt(void);
Expr *parse_expr(void);
const char *parse_name(void);

Typespec *parse_type_base(void)
{
	if (is_token(TOKEN_NAME)) {
		SrcPos pos = token.pos;
		const char **names = 0;
		buf_push(names, token.name);
		next_token();
		while (match_token(TOKEN_DOT)) {
			/*
			 * I think what he means here is all the items
			 * of a struct/union. might remove this later
			 */
			TODO(buf_push(names, parse_name()));
		}
		return new_typespec_name(pos, names, buf_len(names));
	} else if (match_token(TOKEN_LPAREN)) {
		Typespec *type = parse_type();
		expect_token(TOKEN_RPAREN);
		return type;
	} else {
		fatal_error_here("Unexpected token %s in type", token_info());
		return 0;
	}
}

/*
 * TODO can probably remove this function and call parse_type_base directly
 */
Typespec *parse_type(void)
{
	Typespec *type = parse_type_base();
	return type;
}

Expr *parse_expr_unary(void);

Expr *parse_expr_operand(void)
{
	SrcPos pos = token.pos;
	if (is_token(TOKEN_INT)) {
		unsigned long long val = token.int_val;
		next_token();
		return new_expr_int(pos, val);
	} else if (is_token(TOKEN_FLOAT)) {
		const char *start = token.start;
		const char *end = token.end;
		double val = token.float_val;
		next_token();
		return new_expr_float(pos, start, end, val);
	} else if (is_token(TOKEN_STR)) {
		const char *val = token.str_val;
		next_token();
		return new_expr_str(pos, val);
	} else if (is_token(TOKEN_NAME)) {
		const char *name = token.name;
		next_token();
		return new_expr_name(pos, name);
	} else if (match_token(TOKEN_LPAREN)) {
		Expr *expr = parse_expr();
		expect_token(TOKEN_RPAREN);
		return new_expr_paren(pos, expr);
	} else {
		fatal_error_here("Unexpected token %s in expression", token_info());
		return 0;
	}
}

Expr *parse_expr_base(void)
{
	Expr *expr = parse_expr_operand();
	while (is_token(TOKEN_LPAREN)) {
		SrcPos pos = token.pos;
		expect_token(TOKEN_LPAREN);
		Expr **args = 0;
		if (!is_token(TOKEN_RPAREN)) {
			buf_push(args, parse_expr());
			while (match_token(TOKEN_COMMA)) {
				buf_push(args, parse_expr());
			}
		}
		expect_token(TOKEN_RPAREN);
		expr = new_expr_call(pos, expr, args, buf_len(args));
	}
	return expr;
}

unsigned is_unary_op(void)
{
	return is_token(TOKEN_ADD) || is_token(TOKEN_SUB);
}

Expr *parse_expr_unary(void)
{
	if (is_unary_op()) {
		SrcPos pos = token.pos;
		TokenKind op = token.kind;
		next_token();
		return new_expr_unary(pos, op, parse_expr_unary());
	} else {
		return parse_expr_base();
	}
}

unsigned is_mul_op(void)
{
	return TOKEN_FIRST_MUL <= token.kind && token.kind <= TOKEN_LAST_MUL;
}

Expr *parse_expr_mul(void)
{
	Expr *expr = parse_expr_unary();
	while (is_mul_op()) {
		SrcPos pos = token.pos;
		TokenKind op = token.kind;
		next_token();
		expr = new_expr_binary(pos, op, expr, parse_expr_unary());
	}
	return expr;
}

unsigned is_add_op(void)
{
	return TOKEN_FIRST_ADD <= token.kind && token.kind <= TOKEN_LAST_ADD;
}

Expr *parse_expr_add(void)
{
	Expr *expr = parse_expr_mul();
	while (is_add_op()) {
		SrcPos pos = token.pos;
		TokenKind op = token.kind;
		next_token();
		expr = new_expr_binary(pos, op, expr, parse_expr_mul());
	}
	return expr;
}

unsigned is_cmp_op(void)
{
	return (TOKEN_FIRST_CMP <= token.kind && token.kind <= TOKEN_LAST_CMP)
		|| TOKEN_ASSIGN == token.kind;
}

Expr *parse_expr_cmp(void)
{
	Expr *expr = parse_expr_add();
	while (is_cmp_op()) {
		SrcPos pos = token.pos;
		TokenKind op = token.kind;
		next_token();
		expr = new_expr_binary(pos, op, expr, parse_expr_add());
	}
	return expr;
}

Expr *parse_expr_and(void)
{
	Expr *expr = parse_expr_cmp();
	while (match_token(TOKEN_AND_AND)) {
		SrcPos pos = token.pos;
		expr = new_expr_binary(pos, TOKEN_AND_AND, expr, parse_expr_cmp());
	}
	return expr;
}

Expr *parse_expr_or(void)
{
	Expr *expr = parse_expr_and();
	while (match_token(TOKEN_OR_OR)) {
		SrcPos pos = token.pos;
		expr = new_expr_binary(pos, TOKEN_OR_OR, expr, parse_expr_and());
	}
	return expr;
}

Expr *parse_expr(void)
{
	return parse_expr_or();
}

unsigned is_end_of_block(void)
{
	return is_keyword(else_keyword)
		|| is_keyword(elseif_keyword)
		|| is_keyword(end_keyword)
		|| is_keyword(wend_keyword)
		|| is_keyword(next_keyword)	
		|| is_keyword(case_keyword)
		|| is_keyword(loop_keyword);
}

StmtList parse_stmt_block(void)
{
	SrcPos pos = token.pos;
	Stmt **stmts = 0;
	while (!is_token_eof() && !is_end_of_block()) {
		buf_push(stmts, parse_stmt());
	}
	return new_stmt_list(pos, stmts, buf_len(stmts));
}

Stmt *parse_stmt_if(SrcPos pos)
{
	Expr *cond = parse_expr();
	expect_keyword(then_keyword);
	StmtList then_block = parse_stmt_block();
	StmtList else_block = {0};
	ElseIf *elseifs = 0;
	while (match_keyword(elseif_keyword)) {
		Expr *elseif_cond = parse_expr();
		expect_keyword(then_keyword);
		StmtList elseif_block = parse_stmt_block();
		buf_push(elseifs, (ElseIf){elseif_cond, elseif_block});
	}
	if (match_keyword(else_keyword)) {
		else_block = parse_stmt_block();
	}
	expect_keyword(end_keyword);
	expect_keyword(if_keyword);
	return new_stmt_if(pos, cond, then_block, elseifs,
			buf_len(elseifs), else_block);
}

Stmt *parse_stmt_while(SrcPos pos)
{
	Expr *cond = parse_expr();
	Stmt *stmt = new_stmt_while(pos, cond, parse_stmt_block());
	expect_keyword(wend_keyword);
	return stmt;
}

Stmt *parse_stmt_do_while_loop(SrcPos pos)
{
	Expr *cond = parse_expr();
	StmtList block = parse_stmt_block();
	if (!match_keyword(loop_keyword)) {
		fatal_error_here("Expected 'loop' after 'do while' block");
		return 0;
	}
	Stmt *stmt = new_stmt_do_while_loop(pos, cond, block);
	return stmt;
}

Stmt *parse_stmt_do_until_loop(SrcPos pos)
{
	Expr *cond = parse_expr();
	StmtList block = parse_stmt_block();
	if (!match_keyword(loop_keyword)) {
		fatal_error_here("Expected 'loop' after 'do until' block");
		return 0;
	}
	Stmt *stmt = new_stmt_do_until_loop(pos, cond, block);
	return stmt;
}

Stmt *parse_stmt_do(SrcPos pos)
{
	Stmt *stmt = 0;
	StmtList block = (StmtList){0};
	if (match_keyword(until_keyword)) {
		stmt = parse_stmt_do_until_loop(pos);
	} else if (match_keyword(while_keyword)) {
		stmt = parse_stmt_do_while_loop(pos);
	} else {
		block = parse_stmt_block();
		expect_keyword(loop_keyword);
		if (match_keyword(while_keyword)) {
			stmt = new_stmt_do_while(pos, parse_expr(), block);
		} else if(match_keyword(until_keyword)) {
			stmt = new_stmt_do_until(pos, parse_expr(), block);
		} else {
			fatal_error_here("Expected 'loop' then 'while'"
					"or 'until after 'do' block");
			return 0;
		}
	}
	return stmt;
}

unsigned is_assign_op(void)
{
	return TOKEN_FIRST_ASSIGN <= token.kind
		&& token.kind <= TOKEN_LAST_ASSIGN;
}

Stmt *parse_stmt_dim(void)
{
	Stmt *stmt = 0;
	Expr *expr = parse_expr();
	assert(EXPR_NAME == expr->kind);
	expect_keyword(as_keyword);
	Typespec *type = parse_type();
	stmt = new_stmt_dim(expr->pos, expr->name, type);
	assert(STMT_DIM == stmt->kind);
	return stmt;
}

Stmt *parse_simple_stmt(void)
{
	SrcPos pos = token.pos;
	Stmt *stmt = 0;
	Expr *expr = 0;
	expr = parse_expr_unary();
	if (is_assign_op()) {
		TokenKind op = token.kind;
		assert(TOKEN_ASSIGN == op);
		next_token();
		stmt = new_stmt_assign(pos, op, expr, parse_expr());
	} else {
		stmt = new_stmt_expr(pos, expr);
		assert(EXPR_CALL == expr->kind);
	}
	return stmt;
}

#define SET_LIT(i, lit) \
	if (EXPR_UNARY == i->kind) { \
		if (TOKEN_SUB == i->unary.op) \
			lit = -i->unary.expr->int_lit.val; \
		else \
			lit = i->unary.expr->int_lit.val; \
	} else { \
		if (EXPR_INT == i->kind) { \
			lit = i->int_lit.val; \
		} else { \
			assert(EXPR_FLOAT == i->kind); \
			lit = i->float_lit.val; \
		} \
	}
	
Expr *parse_expr_for_cond(SrcPos pos, Stmt *init, Expr *end)
{
	Expr *it = init->assign.left;
	Expr *start = init->assign.right;
	Expr *cond = 0;
	long long lit_start = 0;
	long long lit_end = 0;

	SET_LIT(start, lit_start);
	SET_LIT(end, lit_end);

	if (lit_start <= lit_end)
		cond = new_expr_binary(pos, TOKEN_GTEQ, it, end);
	else
		cond = new_expr_binary(pos, TOKEN_LTEQ, it, end);
	return cond;
}
#undef SET_LIT

Stmt *parse_stmt_for(SrcPos pos) {
	Stmt *stmt = 0;
	Stmt *init = 0;
	Expr *it = 0;
	Expr *cond = 0;
	Expr *inc = 0;
	Stmt *next = 0;
	init = parse_simple_stmt();
	assert(STMT_ASSIGN == init->kind);
	expect_keyword(to_keyword);
	it = init->assign.left;
	assert(EXPR_NAME == it->kind);
	cond = parse_expr_for_cond(pos, init, parse_expr_unary());
	if (TOKEN_LTEQ == cond->binary.op) {
		expect_keyword(step_keyword);
		inc = parse_expr_unary();
	} else {
		if (match_keyword(step_keyword))
			inc = parse_expr_unary();
		else
			inc = new_expr_int(pos, 1);
	}
	next = new_stmt_assign(pos, TOKEN_ASSIGN, it,
			new_expr_binary(pos, TOKEN_ADD, it, inc));
	stmt = new_stmt_for(pos, init, cond, next, parse_stmt_block());
	expect_keyword(next_keyword);
	if (is_token(TOKEN_NAME)) {
		if (it->name == token.name) {
			next_token();
		} else {
			error_here("Expected %s after 'next', found %s",
					it->name, token.name);
			next_token();
		}
	}
	return stmt;
}

/* TODO

   SelectCasePattern parse_select_case_pattern(void)
   {
   Expr *start = parse_expr();
   Expr *end = 0;
   return (SwitchCasePattern){start, end};
   }

   SelectCase parse_stmt_select_case(void)
   {
   SelectCasePattern *patterns = 0;
   unsigned is_default = 0;
   unsigned is_first_case = 1;
   while (is_keyword(case_keyword) || is_keyword(default_keyword)) {
   if (match_keyword(case_keyword)) {
   if (!is_first_case) {
   warning_here("Use comma-separated expressions to match multiple values with one case label");
   is_first_case = 0;
   }
   buf_push(patterns, parse_switch_case_pattern());
   while (match_token(TOKEN_COMMA)) {
   buf_push(patterns, parse_switch_case_pattern());
   }
   } else {
   assert(is_keyword(default_keyword));
   next_token();
   if (is_default) {
   error_here("Duplicate default labels in same switch clause");
   }
   is_default = true;
   }
   expect_token(TOKEN_COLON);
   }
   SrcPos pos = token.pos;
   Stmt **stmts = NULL;
   while (!is_token_eof() && !is_token(TOKEN_RBRACE) && !is_keyword(case_keyword) && !is_keyword(default_keyword)) {
   buf_push(stmts, parse_stmt());
   }
   return (SwitchCase){patterns, buf_len(patterns), is_default, new_stmt_list(pos, stmts, buf_len(stmts))};
   }

   Stmt *parse_stmt_switch(SrcPos pos) {
   Expr *expr = parse_paren_expr();
   SwitchCase *cases = NULL;
   expect_token(TOKEN_LBRACE);
   while (!is_token_eof() && !is_token(TOKEN_RBRACE)) {
   buf_push(cases, parse_stmt_switch_case());
   }
   expect_token(TOKEN_RBRACE);
   return new_stmt_switch(pos, expr, cases, buf_len(cases));
   }

 */

Stmt *parse_stmt(void)
{
	SrcPos pos = token.pos;
	Stmt *stmt = 0;
	if (match_keyword(if_keyword)) {
		stmt = parse_stmt_if(pos);
	} else if (match_keyword(while_keyword)) {
		stmt = parse_stmt_while(pos);
	} else if (match_keyword(do_keyword)) {
		stmt = parse_stmt_do(pos);
	} else if (match_keyword(for_keyword)) {
		stmt = parse_stmt_for(pos);
	} else if (match_keyword(select_keyword)) {
		TODO(stmt = parse_stmt_select_case(pos));
		printf("remove TODO\n");
	} else if (match_keyword(dim_keyword)) {
		stmt = parse_stmt_dim();
	} else {
		stmt = parse_simple_stmt();
	}
	return stmt;
}

const char *parse_name(void)
{
	const char *name = token.name;
	expect_token(TOKEN_NAME);
	return name;
}

Decl *parse_decl_dim(SrcPos pos)
{
	const char *name = parse_name();
	expect_keyword(as_keyword);
	Typespec *type = parse_type();
	return new_decl_dim(pos, name, type, 0);
}

Decl *parse_decl_opt(void)
{
	SrcPos pos = token.pos;
	if (match_keyword(dim_keyword)) {
		return parse_decl_dim(pos);
	} else {
		return 0;
	}
}

Decl *parse_decl(void)
{
	Decl *decl = parse_decl_opt();
	if (!decl) {
		fatal_error_here("Expected declaration keyword, got %s", token_info());
	}
	return decl;
}

Decls *parse_decls(void)
{
	Decl **decls = 0;
	while (!is_token(TOKEN_EOF)) {
		buf_push(decls, parse_decl());
	}
	return new_decls(decls, buf_len(decls));
}