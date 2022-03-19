#ifndef PRINT_AST_H_
#define PRINT_AST_H_

#include "ast.h"

void print_expr(const Expr *expr);
void print_stmt(const Stmt *stmt);
void print_stmts(Stmt **stmts);
void print_typespec(const Typespec *type);
void print_expr(const Expr *e);
void print_exprln(const Expr *e);
void print_newline(void);
void print_stmt_block(const StmtBlock block);
void print_select_case_pattern(const SelectCasePattern scp);
void print_select_case(SelectCase sc);
void print_select_cases(const SelectCase *cases);
void print_stmt(const Stmt *stmt);


#endif
