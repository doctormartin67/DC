const char *dim_keyword;
const char *as_keyword;
const char *if_keyword;
const char *then_keyword;
const char *else_keyword;
const char *elseif_keyword;
const char *end_keyword;
const char *mod_keyword;
const char *true_keyword;
const char *false_keyword;
const char *and_keyword;
const char *or_keyword;
const char *xor_keyword;
const char *while_keyword;
const char *until_keyword;
const char *wend_keyword;
const char *do_keyword;
const char *loop_keyword;
const char *for_keyword;
const char *to_keyword;
const char *step_keyword;
const char *next_keyword;
const char *select_keyword;
const char *case_keyword;
const char *is_keyword;

const char *first_keyword;
const char *last_keyword;

const char *first_op_keyword;
const char *last_op_keyword;

const char **keywords;

typedef enum TokenKind {
	TOKEN_EOF,
	TOKEN_COLON,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_POUND,
	TOKEN_QUESTION,
	TOKEN_KEYWORD,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_STR,
	TOKEN_NAME,
	TOKEN_NEG,
	TOKEN_NOT,
	// Multiplicative precedence
	TOKEN_FIRST_MUL,
	TOKEN_MUL = TOKEN_FIRST_MUL,
	TOKEN_DIV,
	TOKEN_EXP,
	TOKEN_MOD,
	TOKEN_AND,
	TOKEN_LAST_MUL = TOKEN_AND,
	// Additive precedence
	TOKEN_FIRST_ADD,
	TOKEN_ADD = TOKEN_FIRST_ADD,
	TOKEN_SUB,
	TOKEN_LAST_ADD = TOKEN_SUB,
	// Comparative precedence
	TOKEN_FIRST_CMP,
	TOKEN_NOTEQ = TOKEN_FIRST_CMP,
	TOKEN_LT,
	TOKEN_GT,
	TOKEN_LTEQ,
	TOKEN_GTEQ,
	TOKEN_LAST_CMP = TOKEN_GTEQ,
	TOKEN_AND_AND,
	TOKEN_OR_OR,
	TOKEN_XOR_XOR,
	// Assignment operators
	TOKEN_FIRST_ASSIGN,
	TOKEN_ASSIGN = TOKEN_FIRST_ASSIGN,
	TOKEN_LAST_ASSIGN = TOKEN_ASSIGN,
	NUM_TOKEN_KINDS,
} TokenKind;

typedef struct SrcPos {
	const char *name;
	int line;
} SrcPos;

typedef struct Token {
	TokenKind kind;
	SrcPos pos;
	const char *start;
	const char *end;
	union {
		unsigned long long int_val;
		double float_val;
		const char *str_val;
		const char *name;
	};
} Token;

const char *token_kind_name(TokenKind kind);
void warning(SrcPos pos, const char *fmt, ...);
void error(SrcPos pos, const char *fmt, ...);
