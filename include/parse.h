#ifndef PARSE_H_
#define PARSE_H_

#include "ast.h"

Typespec *parse_type(void);
Stmt *parse_stmt(void);
Expr *parse_expr(void);
const char *parse_name(void);
Stmt **parse_stmts(void);

#endif
