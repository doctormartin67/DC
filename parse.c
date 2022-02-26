#include "ast.h"

Typespec *parse_type(void);
Stmt *parse_stmt(void);
Expr *parse_expr(void);
const char *parse_name(void);

Typespec *parse_type_base(void)
{
	if (is_token(TOKEN_NAME)) {
		SrcPos pos = token_pos();
		const char *name = token_name();
		next_token();
		return new_typespec_name(pos, name);
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
	SrcPos pos = token_pos();
	if (is_token(TOKEN_INT)) {
		unsigned long long val = token_int_val();
		next_token();
		return new_expr_int(pos, val);
	} else if (match_keyword(true_keyword)) {
		bool val = true;
		return new_expr_boolean(pos, val);
	} else if (match_keyword(false_keyword)) {
		bool val = false;
		return new_expr_boolean(pos, val);
	} else if (is_token(TOKEN_FLOAT)) {
		const char *start = token_start();
		const char *end = token_end();
		double val = token_float_val();
		next_token();
		return new_expr_float(pos, start, end, val);
	} else if (is_token(TOKEN_STR)) {
		const char *val = token_str_val();
		next_token();
		return new_expr_str(pos, val);
	} else if (is_token(TOKEN_NAME)) {
		const char *name = token_name();
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
		SrcPos pos = token_pos();
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
		SrcPos pos = token_pos();
		TokenKind op = token_kind();
		next_token();
		return new_expr_unary(pos, op, parse_expr_unary());
	} else {
		return parse_expr_base();
	}
}

unsigned is_mul_op(void)
{
	return (TOKEN_FIRST_MUL <= token_kind() && token_kind() <= TOKEN_LAST_MUL)
		|| is_a_keyword(mod_keyword);
}

Expr *parse_expr_mul(void)
{
	Expr *expr = parse_expr_unary();
	while (is_mul_op()) {
		SrcPos pos = token_pos();
		TokenKind op = token_kind();
		if (TOKEN_KEYWORD == op && is_a_keyword(mod_keyword))
			op = TOKEN_MOD;
		next_token();
		expr = new_expr_binary(pos, op, expr, parse_expr_unary());
	}
	return expr;
}

unsigned is_add_op(void)
{
	return TOKEN_FIRST_ADD <= token_kind()
		&& token_kind() <= TOKEN_LAST_ADD;
}

Expr *parse_expr_add(void)
{
	Expr *expr = parse_expr_mul();
	while (is_add_op()) {
		SrcPos pos = token_pos();
		TokenKind op = token_kind();
		next_token();
		expr = new_expr_binary(pos, op, expr, parse_expr_mul());
	}
	return expr;
}

unsigned is_cmp_op(void)
{
	return (TOKEN_FIRST_CMP <= token_kind() && token_kind() <= TOKEN_LAST_CMP)
		|| TOKEN_ASSIGN == token_kind();
}

Expr *parse_expr_cmp(void)
{
	Expr *expr = parse_expr_add();
	while (is_cmp_op()) {
		SrcPos pos = token_pos();
		TokenKind op = token_kind();
		next_token();
		expr = new_expr_binary(pos, op, expr, parse_expr_add());
	}
	return expr;
}

Expr *parse_expr_and(void)
{
	Expr *expr = parse_expr_cmp();
	while (match_keyword(and_keyword)) {
		SrcPos pos = token_pos();
		expr = new_expr_binary(pos, TOKEN_AND_AND, expr, parse_expr_cmp());
	}
	return expr;
}

Expr *parse_expr_xor(void)
{
	Expr *expr = parse_expr_and();
	while (match_keyword(xor_keyword)) {
		SrcPos pos = token_pos();
		expr = new_expr_binary(pos, TOKEN_XOR_XOR, expr, parse_expr_and());
	}
	return expr;
}

Expr *parse_expr_or(void)
{
	Expr *expr = parse_expr_xor();
	while (match_keyword(or_keyword)) {
		SrcPos pos = token_pos();
		expr = new_expr_binary(pos, TOKEN_OR_OR, expr, parse_expr_xor());
	}
	return expr;
}

Expr *parse_expr(void)
{
	return parse_expr_or();
}

unsigned is_end_of_block(void)
{
	return is_a_keyword(else_keyword)
		|| is_a_keyword(elseif_keyword)
		|| is_a_keyword(end_keyword)
		|| is_a_keyword(wend_keyword)
		|| is_a_keyword(next_keyword)	
		|| is_a_keyword(case_keyword)
		|| is_a_keyword(loop_keyword);
}

StmtBlock parse_stmt_block(void)
{
	SrcPos pos = token_pos();
	Stmt **stmts = 0;
	while (!is_token_eof() && !is_end_of_block()) {
		buf_push(stmts, parse_stmt());
	}
	return new_StmtBlock(pos, stmts, buf_len(stmts));
}

Stmt *parse_stmt_if(SrcPos pos)
{
	Expr *cond = parse_expr();
	expect_keyword(then_keyword);
	StmtBlock then_block = parse_stmt_block();
	StmtBlock else_block = {0};
	ElseIf *elseifs = 0;
	while (match_keyword(elseif_keyword)) {
		Expr *elseif_cond = parse_expr();
		expect_keyword(then_keyword);
		StmtBlock elseif_block = parse_stmt_block();
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
	assert(STMT_WHILE == stmt->kind);
	return stmt;
}

Stmt *parse_stmt_do_while_loop(SrcPos pos)
{
	Expr *cond = parse_expr();
	StmtBlock block = parse_stmt_block();
	if (!match_keyword(loop_keyword)) {
		fatal_error_here("Expected 'loop' after 'do while' block");
		return 0;
	}
	Stmt *stmt = new_stmt_do_while_loop(pos, cond, block);
	assert(STMT_DO_WHILE_LOOP == stmt->kind);
	return stmt;
}

Stmt *parse_stmt_do_until_loop(SrcPos pos)
{
	Expr *cond = parse_expr();
	StmtBlock block = parse_stmt_block();
	if (!match_keyword(loop_keyword)) {
		fatal_error_here("Expected 'loop' after 'do until' block");
		return 0;
	}
	Stmt *stmt = new_stmt_do_until_loop(pos, cond, block);
	assert(STMT_DO_UNTIL_LOOP == stmt->kind);
	return stmt;
}

Stmt *parse_stmt_do(SrcPos pos)
{
	Stmt *stmt = 0;
	StmtBlock block = (StmtBlock){0};
	if (match_keyword(until_keyword)) {
		stmt = parse_stmt_do_until_loop(pos);
		assert(STMT_DO_UNTIL_LOOP == stmt->kind);
	} else if (match_keyword(while_keyword)) {
		stmt = parse_stmt_do_while_loop(pos);
		assert(STMT_DO_WHILE_LOOP == stmt->kind);
	} else {
		block = parse_stmt_block();
		expect_keyword(loop_keyword);
		if (match_keyword(while_keyword)) {
			stmt = new_stmt_do_while(pos, parse_expr(), block);
			assert(STMT_DO_WHILE == stmt->kind);
		} else if(match_keyword(until_keyword)) {
			stmt = new_stmt_do_until(pos, parse_expr(), block);
			assert(STMT_DO_UNTIL == stmt->kind);
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
	return TOKEN_FIRST_ASSIGN <= token_kind()
		&& token_kind() <= TOKEN_LAST_ASSIGN;
}

Stmt *parse_stmt_dim(void)
{
	Stmt *stmt = 0;
	Dim *dims = 0;
	size_t num_dims = 1;
	Expr *expr = parse_expr();
	assert(EXPR_NAME == expr->kind);
	expect_keyword(as_keyword);
	Typespec *type = parse_type();
	buf_push(dims, (Dim){expr->name, type});
	while (match_token(TOKEN_COMMA)) {
		num_dims++;
		expr = parse_expr();
		assert(EXPR_NAME == expr->kind);
		expect_keyword(as_keyword);	
		type = parse_type();
		buf_push(dims, (Dim){expr->name, type});
	}
	stmt = new_stmt_dim(expr->pos, dims, num_dims);
	assert(STMT_DIM == stmt->kind);
	return stmt;
}

Stmt *parse_simple_stmt(void)
{
	SrcPos pos = token_pos();
	Stmt *stmt = 0;
	Expr *expr = 0;
	expr = parse_expr_unary();
	if (is_assign_op()) {
		TokenKind op = token_kind();
		assert(TOKEN_ASSIGN == op);
		next_token();
		stmt = new_stmt_assign(pos, op, expr, parse_expr());
	} else {
		stmt = new_stmt_expr(pos, expr);
		if (EXPR_NAME == expr->kind) {
			fatal_error_here("Unknown name '%s'", expr->name);
		}
		assert(EXPR_CALL == expr->kind);
	}
	return stmt;
}

#if 0
	in a for loop the iterator has to be declared beforehand
	if the start value is bigger than the end value (without the use of
		step -1 for example) then the condition is false and the for
		loop exits
#endif
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
	cond = new_expr_binary(pos, TOKEN_LTEQ, it, parse_expr_unary());
	if (match_keyword(step_keyword)) {
		inc = parse_expr_unary();
	} else {
		inc = new_expr_int(pos, 1);
	}
	next = new_stmt_assign(pos, TOKEN_ASSIGN, it,
			new_expr_binary(pos, TOKEN_ADD, it, inc));
	stmt = new_stmt_for(pos, init, cond, next, parse_stmt_block());
	expect_keyword(next_keyword);
	if (is_token(TOKEN_NAME)) {
		if (it->name == token_name()) {
			next_token();
		} else {
			error_here("Expected %s after 'next', found %s",
					it->name, token_name());
			next_token();
		}
	}
	return stmt;
}

SelectCasePattern parse_select_case_pattern(void) {
	SelectCasePattern scp = (SelectCasePattern){0};
	Expr *start = 0;
	Expr *end = 0;
	if (match_keyword(is_keyword)) {
		assert(is_cmp_op());	
		scp.kind = PATTERN_IS;
		scp.is_pattern.op = token_kind();
		next_token();
		scp.is_pattern.expr = parse_expr();
	} else {
		start = parse_expr();
		if (match_keyword(to_keyword)) {
			scp.kind = PATTERN_TO;
			end = parse_expr();
			scp.to_pattern.start = start;
			scp.to_pattern.end = end;
		} else {
			scp.kind = PATTERN_LIT;
			scp.expr = start;
		}
	}
	return scp;
}

SelectCase parse_stmt_select_case(void)
{
	SelectCasePattern *patterns = 0;
	unsigned is_default = 0;
	unsigned is_first_case = 1;
	while (match_keyword(case_keyword)) {
		if (match_keyword(else_keyword)) {
			if (is_default) {
				error_here("Duplicate default labels in same"
						"switch clause");
			}
			is_default = 1;
		} else {
			if (!is_first_case) {
				warning_here("Use comma-separated expressions"
						"to match multiple values with"
						"one case label");
				is_first_case = 0;
			}
			buf_push(patterns, parse_select_case_pattern());
			while (match_token(TOKEN_COMMA)) {
				buf_push(patterns,
						parse_select_case_pattern());
			}
			if (is_token(TOKEN_COLON)) next_token();
		}
	}
	SrcPos pos = token_pos();
	Stmt **stmts = 0;
	while (!is_token_eof()
			&& !is_a_keyword(case_keyword)
			&& !is_a_keyword(end_keyword)) {
		buf_push(stmts, parse_stmt());
	}
	return (SelectCase){patterns, buf_len(patterns), is_default,
		new_StmtBlock(pos, stmts, buf_len(stmts))};
}

Stmt *parse_stmt_select(SrcPos pos)
{
	expect_keyword(case_keyword);
	Expr *expr = parse_expr();
	assert(EXPR_NAME == expr->kind);
	SelectCase *cases = 0;
	while (!is_token_eof() && !is_a_keyword(end_keyword)) {
		buf_push(cases, parse_stmt_select_case());
	}
	expect_keyword(end_keyword);
	expect_keyword(select_keyword);
	return new_stmt_select_case(pos, expr, cases, buf_len(cases));
}


Stmt *parse_stmt(void)
{
	SrcPos pos = token_pos();
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
		stmt = parse_stmt_select(pos);
	} else if (match_keyword(dim_keyword)) {
		stmt = parse_stmt_dim();
	} else {
		stmt = parse_simple_stmt();
	}
	return stmt;
}

Stmt **parse_stmts(void)
{
	Stmt **stmts = 0;
	while (!is_token(TOKEN_EOF)) {
		buf_push(stmts, parse_stmt());
	}
	return stmts;
}

const char *parse_name(void)
{
	const char *name = token_name();
	expect_token(TOKEN_NAME);
	return name;
}
