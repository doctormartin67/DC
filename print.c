static int indent;
void print_expr(Expr *expr);
void print_stmt(Stmt *stmt);
void print_decl(Decl *decl);

void print_typespec(Typespec *type)
{
	Typespec *t = type;
	switch (t->kind) {
		case TYPESPEC_NAME:
			printf("%s", t->names[0]);
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

void print_expr(Expr *e)
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
					printf(",");
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
			printf("(%c ", e->unary.op);
			print_expr(e->unary.expr);
			printf(")");
			break;
		case EXPR_BINARY:
			printf("(%c ", e->binary.op);
			print_expr(e->binary.left);
			printf(" ");
			print_expr(e->binary.right);
			printf(")");
			break;
		case EXPR_MODIFY:
			printf("TODO");
			break;
		case EXPR_NEW:
			printf("TODO");
			break;
	}
}

void print_exprln(Expr *e)
{
	print_expr(e);
	printf("\n");
}

void print_newline(void)
{
	printf("\n%.*s", 2*indent, "                                                                      ");
}

void print_stmt_block(StmtList block)
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

void print_stmt(Stmt *stmt)
{
	Stmt *s = stmt;
	switch (s->kind) {
		case STMT_DECL:
			print_decl(s->decl);
			break;
		case STMT_BLOCK:
			print_stmt_block(s->block);
			break;
		case STMT_IF:
			printf("(If ");
			print_expr(s->if_stmt.cond);
			indent++;
			printf("Then");
			print_newline();
			print_stmt_block(s->if_stmt.then_block);
			for (ElseIf *it = s->if_stmt.elseifs;
					it != s->if_stmt.elseifs
					+ s->if_stmt.num_elseifs; it++) {
				print_newline();
				printf("ElseIf ");
				print_expr(it->cond);
				printf("Then");
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
			printf("End If)");
			break;
		case STMT_WHILE:
			printf("(While ");
			print_expr(s->while_stmt.cond);
			indent++;
			print_newline();
			print_stmt_block(s->while_stmt.block);
			indent--;
			printf("Wend)");
			break;
		case STMT_DO_WHILE:
			printf("(Do While ");
			print_expr(s->while_stmt.cond);
			indent++;
			print_newline();
			print_stmt_block(s->while_stmt.block);
			indent--;
			printf("Loop)");
			break;
		case STMT_DO_UNTIL:
			printf("(Do Until ");
			print_expr(s->while_stmt.cond);
			indent++;
			print_newline();
			print_stmt_block(s->while_stmt.block);
			indent--;
			printf("Loop)");
			break;
		case STMT_FOR:
			printf("(For ");
			print_stmt(s->for_stmt.init);
			print_expr(s->for_stmt.cond);
			print_stmt(s->for_stmt.next);
			indent++;
			print_newline();
			print_stmt_block(s->for_stmt.block);
			indent--;
			printf("Next)");
			break;
		case STMT_SELECT_CASE:
			printf("TODO");
			break;
		case STMT_ASSIGN:
			printf("(%s ", token_kind_names[s->assign.op]);
			print_expr(s->assign.left);
			if (s->assign.right) {
				printf(" ");
				print_expr(s->assign.right);
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

void print_decl(Decl *decl)
{
	Decl *d = decl;
	switch (d->kind) {
		case DECL_VAR:
			printf("(Dim %s As", d->name);
			if (d->var.type) {
				print_typespec(d->var.type);
			} else {
				printf("nil");
			}
			printf(" ");
			if (d->var.expr) {
				print_expr(d->var.expr);
			} else {
				printf("nil");
			}
			printf(")");
			break;
		default:
			assert(0);
			break;
	}
}


void test_print(void)
{
	// Expressions
	SrcPos pos = (SrcPos){0};
	Expr *es[] = {
		new_expr_binary(pos, '+', new_expr_int(pos, 1),
				new_expr_int(pos, 2)),
		new_expr_call(pos, new_expr_name(pos, "func"),
				(Expr*[]){new_expr_int(pos, 42)}, 1),
	};
	for (Expr **it = es; it != es + sizeof(es)/sizeof(*es); it++) {
		print_expr(*it);
		printf("\n");
	}
}
