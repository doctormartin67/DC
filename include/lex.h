#ifndef LEX_H_
#define LEX_H_

#include <stdlib.h>

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
const char *project_keyword;
const char *is_keyword;

const char *first_keyword;
const char *last_keyword;

const char *first_op_keyword;
const char *last_op_keyword;

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

void init_keywords(void);
unsigned is_keyword_name(const char *name);
const char *token_kind_name(TokenKind kind);
void next_token(void);
void init_stream(const char *name, const char *buf);
void clear_stream(void);
unsigned is_token(TokenKind kind);
unsigned is_token_eof(void);
unsigned is_a_keyword(const char *name);
unsigned match_keyword(const char *name);
unsigned expect_keyword(const char *name);
unsigned match_token(TokenKind kind);
unsigned expect_token(TokenKind kind);
SrcPos token_pos(void);
const char *token_name(void);
TokenKind token_kind(void);
unsigned long long token_int_val(void);
double token_float_val(void);
const char *token_str_val(void);
const char *token_start(void);
const char *token_end(void);
const char *token_info(void);

void warning(SrcPos pos, const char *fmt, ...);
void syntax_error(SrcPos pos, const char *fmt, ...);
#define error_here(...) (syntax_error(token_pos(), __VA_ARGS__))
#define warning_here(...) (syntax_error(token_pos(), __VA_ARGS__))

#endif
