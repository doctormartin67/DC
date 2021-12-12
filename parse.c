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
	return TOKEN_FIRST_CMP <= token.kind && token.kind <= TOKEN_LAST_CMP;
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
	return match_keyword(else_keyword)
	|| match_keyword(elseif_keyword)
	|| match_keyword(end_keyword)
	|| match_keyword(wend_keyword)
	|| match_keyword(next_keyword)	
	|| match_keyword(case_keyword);
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
	StmtList then_block = parse_stmt_block();
	expect_keyword(then_keyword);
	StmtList else_block = {0};
	ElseIf *elseifs = 0;
	while (!match_keyword(end_keyword)) {
		if (match_keyword(else_keyword)) {
			else_block = parse_stmt_block();
			break;
		}
		expect_keyword(elseif_keyword);
		Expr *elseif_cond = parse_expr();
		expect_keyword(then_keyword);
		StmtList elseif_block = parse_stmt_block();
		buf_push(elseifs, (ElseIf){elseif_cond, elseif_block});
	}
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

Stmt *parse_stmt_do_while(SrcPos pos)
{
	StmtList block = parse_stmt_block();
	if (!match_keyword(while_keyword)) {
		fatal_error_here("Expected 'while' after 'do' block");
		return 0;
	}
	Stmt *stmt = new_stmt_do_while(pos, parse_expr(), block);
	expect_keyword(loop_keyword);
	return stmt;
}

unsigned is_assign_op(void)
{
	return TOKEN_FIRST_ASSIGN <= token.kind
		&& token.kind <= TOKEN_LAST_ASSIGN;
}

/* TODO
 * probably remove function below
 */
Stmt *parse_simple_stmt(void)
{
	return 0;
}

/* TODO
Stmt *parse_stmt_for(SrcPos pos) {
	Stmt *init = NULL;
	Expr *cond = NULL;
	Stmt *next = NULL;
	if (!is_token(TOKEN_LBRACE)) {
		expect_token(TOKEN_LPAREN);
		if (!is_token(TOKEN_SEMICOLON)) {
			init = parse_simple_stmt();
		}
		if (match_token(TOKEN_SEMICOLON)) {
			if (!is_token(TOKEN_SEMICOLON)) {
				cond = parse_expr();
			}
			if (match_token(TOKEN_SEMICOLON)) {
				if (!is_token(TOKEN_RPAREN)) {
					next = parse_simple_stmt();
					if (next->kind == STMT_INIT) {
						error_here("Init statements not allowed in for-statement's next clause");
					}
				}
			}
		}
		expect_token(TOKEN_RPAREN);
	}
	return new_stmt_for(pos, init, cond, next, parse_stmt_block());
}


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
		stmt = parse_stmt_do_while(pos);
	} else if (match_keyword(for_keyword)) {
		TODO(stmt = parse_stmt_for(pos));
	} else if (match_keyword(select_keyword)) {
		TODO(stmt = parse_stmt_select_case(pos));
	} else if (match_token(TOKEN_COLON)) {
		stmt = new_stmt_label(pos, parse_name());
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
	if (match_token(TOKEN_ASSIGN)) {
		Expr *expr = parse_expr();
		return new_decl_var(pos, name, 0, expr);
	} else {
		fatal_error_here("Expected : or = after var, got %s", token_info());
		return 0;
	}
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
