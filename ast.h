typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Decl Decl;
typedef struct Typespec Typespec;

typedef struct StmtList {
	SrcPos pos;
	Stmt **stmts;
	size_t num_stmts;
} StmtList;

typedef enum TypespecKind {
	TYPESPEC_NONE,
	TYPESPEC_NAME,
	TYPESPEC_FUNC,
} TypespecKind;

struct Typespec {
	TypespecKind kind;
	SrcPos pos;
	Typespec *base;
	union {
		const char *name;
		struct {
			Typespec **args;
			size_t num_args;
		} func;
	};
};

typedef struct FuncParam {
	SrcPos pos;
	const char *name;
	Typespec *type;
} FuncParam;

typedef enum DeclKind {
	DECL_NONE,
	DECL_DIM,
} DeclKind;

struct Decl {
	DeclKind kind;
	SrcPos pos;
	const char *name;
	union {
		struct {
			Typespec *type;
			Expr *expr;
		} dim;
	};
};

typedef struct Decls {
	Decl **decls;
	size_t num_decls;
} Decls;

typedef enum ExprKind {
	EXPR_NONE,
	EXPR_PAREN,
	EXPR_INT,
	EXPR_FLOAT,
	EXPR_STR,
	EXPR_NAME,
	EXPR_CALL,
	EXPR_INDEX,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_MODIFY,
	EXPR_NEW,
} ExprKind;

struct Expr {
	ExprKind kind;
	SrcPos pos;
	union {
		struct {
			Expr *expr;
		} paren;
		struct {
			unsigned long long val;
		} int_lit;
		struct {
			const char *start;
			const char *end;
			double val;
		} float_lit;
		struct {
			const char *val;
		} str_lit;
		const char *name;
		struct {
			TokenKind op;
			Expr *expr;
		} unary;
		struct {
			TokenKind op;
			Expr *left;
			Expr *right;
		} binary;
		struct {
			Expr *expr;
			Expr **args;
			size_t num_args;            
		} call;
		struct {
			Expr *expr;
			Expr *index;
		} index;
	};
};

typedef struct ElseIf {
	Expr *cond;
	StmtList block;
} ElseIf;

typedef enum PatternKind {
	PATTERN_NONE,
	PATTERN_LIT,
	PATTERN_TO,
	PATTERN_IS,
} PatternKind;

typedef struct SelectCasePattern {
	PatternKind kind;
	union {
		Expr *expr;
		struct {
			Expr *start;
			Expr *end;
		} to_pattern;
		struct {
			TokenKind op;
			Expr *expr;
		} is_pattern;
	};
} SelectCasePattern;

typedef struct SelectCase {
	SelectCasePattern *patterns;
	size_t num_patterns;
	unsigned is_default;
	StmtList block;
} SelectCase;

typedef enum StmtKind {
	STMT_NONE,
	STMT_DECL,
	STMT_BLOCK,
	STMT_IF,
	STMT_WHILE,
	STMT_DO_WHILE,
	STMT_DO_UNTIL,
	STMT_DO_WHILE_LOOP,
	STMT_DO_UNTIL_LOOP,
	STMT_FOR,
	STMT_SELECT_CASE,
	STMT_ASSIGN,
	STMT_DIM,
	STMT_EXPR,
	STMT_NOTE,
} StmtKind;

struct Stmt {
	StmtKind kind;
	SrcPos pos;
	union {
		Expr *expr;
		Decl *decl;
		struct {
			Stmt *dim;
			Expr *cond;
			StmtList then_block;
			ElseIf *elseifs;
			size_t num_elseifs;
			StmtList else_block;            
		} if_stmt;
		struct {
			Expr *cond;
			StmtList block;
		} while_stmt;
		struct {
			Expr *cond;
			StmtList block;
		} until_stmt;
		struct {
			Stmt *dim;
			Expr *cond;
			Stmt *next;
			StmtList block;
		} for_stmt;
		struct {
			Expr *expr;
			SelectCase *cases;
			size_t num_cases;            
		} select_case_stmt;
		StmtList block;
		struct {
			TokenKind op;
			Expr *left;
			Expr *right;
		} assign;
		struct {
			const char *name;
			Typespec *type;
		} dim;
	};
};
