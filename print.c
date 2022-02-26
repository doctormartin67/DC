static int indent;
void print_expr(const Expr *expr);
void print_stmt(const Stmt *stmt);

void print_typespec(const Typespec *type)
{
	const Typespec *t = type;
	switch (t->kind) {
		case TYPESPEC_NAME:
			printf("%s", t->name);
			break;
		case TYPESPEC_FUNC:
			printf("(func (");
			for (Typespec **it = t->func.args; it != t->func.args
					+ t->func.num_args; it++) {
				printf(" ");
				print_typespec(*it);
			}
			printf(" ) ");
			printf(")");
			break;
		default:
			assert(0);
			break;
	}
}

void print_expr(const Expr *e)
{
	switch (e->kind) {
		case EXPR_NONE:
			assert(0);
			break;
		case EXPR_PAREN:
			printf("(");
			print_expr(e->paren.expr);
			printf(")");
			break;
		case EXPR_BOOLEAN:
			if (e->boolean_lit.val) {
				printf("True");
			} else {
				printf("False");
			}
			break;
		case EXPR_INT:
			printf("%lld", e->int_lit.val);
			break;
		case EXPR_FLOAT:
			printf("%f", e->float_lit.val);
			break;
		case EXPR_STR:
			printf("\"%s\"", e->str_lit.val);
			break;
		case EXPR_NAME:
			printf("%s", e->name);
			break;
		case EXPR_CALL:
			printf("(call ");
			print_expr(e->call.expr);
			printf("(");
			for (size_t i = 0; i < e->call.num_args; i++) {
				print_expr(e->call.args[i]);	
				if (i != e->call.num_args - 1)
					printf(", ");
			}
			printf("))");
			break;
		case EXPR_INDEX:
			printf("(index ");
			print_expr(e->index.expr);
			printf(" ");
			print_expr(e->index.index);
			printf(")");
			break;
		case EXPR_UNARY:
			printf("(%s", token_kind_name(e->unary.op));
			print_expr(e->unary.expr);
			printf(")");
			break;
		case EXPR_BINARY:
			printf("(%s ", token_kind_name(e->binary.op));
			print_expr(e->binary.left);
			printf(" ");
			print_expr(e->binary.right);
			printf(")");
			break;
		default:
			assert(0);
			break;
	}
}

void print_exprln(const Expr *e)
{
	print_expr(e);
	printf("\n");
}

void print_newline(void)
{
	printf("\n%.*s", 2*indent, "                                                                      ");
}

void print_stmt_block(const StmtBlock block)
{
	printf("(block");
	indent++;
	for (Stmt **it = block.stmts; it != block.stmts + block.num_stmts;
			it++) {
		print_newline();
		print_stmt(*it);
	}
	indent--;
	printf(")");
}

void print_select_case_pattern(const SelectCasePattern scp)
{
	switch (scp.kind) {
		case PATTERN_NONE:
			assert(0);
			break;
		case PATTERN_LIT:
			print_expr(scp.expr);	
			break;
		case PATTERN_TO:
			print_expr(scp.to_pattern.start);	
			printf(" To ");
			print_expr(scp.to_pattern.end);
			break;
		case PATTERN_IS:
			printf("Is ");
			printf("%s ", token_kind_name(scp.is_pattern.op));
			print_expr(scp.is_pattern.expr);
			break;
		default:
			assert(0);
			break;
	}
}

void print_select_case(const SelectCase sc)
{
	print_newline();
	if (sc.is_default)
		printf("(Case Else ");
	else
		printf("(Case ");
	for (size_t i = 0; i < sc.num_patterns; i++) {
		print_select_case_pattern(sc.patterns[i]);
		if (i != sc.num_patterns - 1) {
			printf(", ");
		}
	}
	indent++;
	print_newline();
	print_stmt_block(sc.block);
	indent--;
	printf(")");
}

void print_stmt(const Stmt *stmt)
{
	assert(stmt);
	const Stmt *s = stmt;
	switch (s->kind) {
		case STMT_BLOCK:
			print_stmt_block(s->block);
			break;
		case STMT_IF:
			printf("(If ");
			print_expr(s->if_stmt.cond);
			indent++;
			printf(" Then");
			print_newline();
			print_stmt_block(s->if_stmt.then_block);
			for (ElseIf *it = s->if_stmt.elseifs;
					it != s->if_stmt.elseifs
					+ s->if_stmt.num_elseifs; it++) {
				print_newline();
				printf("ElseIf ");
				print_expr(it->cond);
				printf(" Then");
				print_newline();
				print_stmt_block(it->block);
			}
			if (s->if_stmt.else_block.num_stmts != 0) {
				print_newline();
				printf("Else ");
				print_newline();
				print_stmt_block(s->if_stmt.else_block);
			}
			indent--;
			print_newline();
			printf("End If)");
			break;
		case STMT_WHILE:
			printf("(While ");
			print_expr(s->while_stmt.cond);
			indent++;
			print_newline();
			print_stmt_block(s->while_stmt.block);
			indent--;
			print_newline();
			printf("Wend)");
			break;
		case STMT_DO_WHILE:
			printf("(Do ");
			indent++;
			print_newline();
			print_stmt_block(s->while_stmt.block);
			indent--;
			print_newline();
			printf("Loop While ");
			print_expr(s->while_stmt.cond);
			break;
		case STMT_DO_UNTIL:
			printf("(Do ");
			indent++;
			print_newline();
			print_stmt_block(s->until_stmt.block);
			indent--;
			print_newline();
			printf("Loop Until ");
			print_expr(s->until_stmt.cond);
			break;
		case STMT_DO_WHILE_LOOP:
			printf("(Do While ");
			print_expr(s->while_stmt.cond);
			indent++;
			print_newline();
			print_stmt_block(s->while_stmt.block);
			indent--;
			print_newline();
			printf("Loop)");
			break;
		case STMT_DO_UNTIL_LOOP:
			printf("(Do Until ");
			print_expr(s->until_stmt.cond);
			indent++;
			print_newline();
			print_stmt_block(s->until_stmt.block);
			indent--;
			print_newline();
			printf("Loop)");
			break;
		case STMT_FOR:
			printf("(For ");
			print_stmt(s->for_stmt.dim);
			print_expr(s->for_stmt.cond);
			print_stmt(s->for_stmt.next);
			indent++;
			print_newline();
			print_stmt_block(s->for_stmt.block);
			indent--;
			print_newline();
			printf("Next %s)", s->for_stmt.dim->assign.left->name);
			break;
		case STMT_SELECT_CASE:
			printf("(Select Case ");
			print_expr(s->select_case_stmt.expr);
			indent++;
			for (size_t i = 0; i < s->select_case_stmt.num_cases;
					i++) {
				print_select_case(
						s->select_case_stmt.cases[i]);
			}
			indent--;
			print_newline();
			printf("End Select)");
			break;
		case STMT_ASSIGN:
			printf("(%s ", token_kind_name(s->assign.op));
			print_expr(s->assign.left);
			if (s->assign.right) {
				printf(" ");
				print_expr(s->assign.right);
			}
			printf(")");
			break;
		case STMT_DIM:
			printf("(Dim ");
			for (size_t i = 0; i < s->dim_stmt.num_dims; i++) {
				printf("(%s As ", s->dim_stmt.dims[i].name);
				if (s->dim_stmt.dims[i].type) {
					print_typespec(s->dim_stmt.dims[i].type);
				} else {
					printf("nil");
				}
				printf(")");
				if (i != s->dim_stmt.num_dims - 1)
					printf(", ");
			}
			printf(")");
			break;
		case STMT_EXPR:
			print_expr(s->expr);
			break;
		default:
			assert(0);
			break;
	}
}
