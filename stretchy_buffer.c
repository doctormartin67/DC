/*
 * one of the most useful tricks in C ever invented.
 * push and pop element in an array of arbitrary type
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
typedef struct {
	size_t len;
	size_t cap;
	char buf[0];
} BufHdr;

/*
 * convention: single underscores are for public use, double underscores are
 * for private internal use
 */
#define BUF(x) x
#define buf__hdr(b) ((BufHdr *)((char *)b - offsetof(BufHdr, buf)))
#define buf__fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define buf__fit(b, n) (buf__fits((b), (n)) ? 0 : \
		((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_push(b, ...) (buf__fit((b), 1), (b)[buf__hdr(b)->len++] = \
		(__VA_ARGS__))
#define buf_end(b) ((b) + buf_len(b))
#define buf_free(b) ((b) ? free(buf__hdr(b)), (b) = 0 : 0)

void fatal(const char *fmt, ...);
void syntax_error(const char *fmt, ...);
void test_buf(void);
void test_str_intern(void);
void test_lex(void);
void *buf__grow(const void *buf, size_t new_len, size_t elem_size);

const char *str_intern_range(const char *start, const char *end);
const char *str_intern(const char *str);

void scan_char(void);
void next_token(void);


void fatal(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("FATAL :");
	vprintf(fmt, args);	
	printf("\n");
	va_end(args);
	exit(1);
}

void syntax_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("Syntax Error: ");
	vprintf(fmt, args);	
	printf("\n");
	va_end(args);
}

void *buf__grow(const void *buf, size_t new_len, size_t elem_size)
{
	assert(buf_cap(buf) <= (SIZE_MAX - 1)/2);
	size_t new_cap = MAX(1 + 2 * buf_cap(buf), new_len);
	assert(new_len <= new_cap);
	assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf))/elem_size);
	size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
	BufHdr *new_hdr = 0;
	if (buf) {
		new_hdr = realloc(buf__hdr(buf), new_size);
	} else {
		new_hdr = malloc(new_size);
		new_hdr->len = 0;
	}
	new_hdr->cap = new_cap;
	return new_hdr->buf;
}

void test_buf(void)
{
	int *crap = 0;
	assert(buf_len(crap) == 0);
	enum {N = 1024};
	for (unsigned i = 0; i < N; i++)
		buf_push(crap, i);
	assert(buf_len(crap) == N);
	for (unsigned i = 0; i < buf_len(crap); i++)
		assert(crap[i] == i);
	buf_free(crap);
	assert(crap == 0);
	assert(buf_len(crap) == 0);
}

/*
 * another cool technique, string interning
 */
void test_str_intern(void)
{
	char x[] = "hello";
	char y[] = "hello";
	char z[] = "hello!";
	assert(x != y);
	assert(str_intern(x) == str_intern(y));
	const char *px = str_intern(x);
	const char *pz = str_intern(z);
	assert(px != pz);
}

typedef struct {
	size_t len;
	const char *str;
} Intern;

static Intern *interns;

const char *str_intern_range(const char *start, const char *end)
{
	size_t len = end - start;
	for (Intern *it = interns; it != buf_end(interns); it++) {
		if (it->len == len && strncmp(it->str, start, len) == 0) {
			return it->str;
		}
	}
	char *str = malloc(len + 1);
	memcpy(str, start, len);
	str[len] = '\0';
	buf_push(interns, (Intern){len, str});
	return str;
}

const char *str_intern(const char *str)
{
	return str_intern_range(str, str + strlen(str));
}

/*
 * parse tokens:
 */

/* lexing: translating char stream to token stream
 * f.e. 1234 (x+y) translates into '1234' '(' 'x' '+' 'y' ')'
 */

typedef enum {
	TOKEN_EOF = 0,
	TOKEN_LAST_CHAR = 127,
	TOKEN_INT, 
	TOKEN_FLOAT, 
	TOKEN_STR,
	TOKEN_NAME,
	TOKEN_LSHIFT,
	TOKEN_RSHIFT,
	TOKEN_EQ,
	TOKEN_NOTEQ,
	TOKEN_LTEQ,
	TOKEN_GTEQ,
	TOKEN_AND,
	TOKEN_OR,
	TOKEN_INC,
	TOKEN_DEC,
	TOKEN_ASSIGN_COLON,
	TOKEN_ASSIGN_ADD,
	TOKEN_ASSIGN_SUB,
	TOKEN_ASSIGN_AND,
	TOKEN_ASSIGN_OR,
	TOKEN_ASSIGN_XOR,
	TOKEN_ASSIGN_LSHIFT,
	TOKEN_ASSIGN_RSHIFT,
	TOKEN_ASSIGN_MUL,
	TOKEN_ASSIGN_DIV,
	TOKEN_ASSIGN_MOD,

} TokenKind;

typedef enum {
	TOKENMOD_NONE,
	TOKENMOD_BIN,
	TOKENMOD_CHAR,
	TOKENMOD_OCT,
	TOKENMOD_HEX,
} TokenMod;

typedef struct {
	TokenKind kind;
	TokenMod mod;
	const char *start;
	const char *end;
	union {
		uint64_t val_int;
		double val_float;
		const char *val_str;
		const char *name;
	};
} Token;

static Token token;
static const char *stream;

size_t copy_token_kind_str(char *dest, size_t dest_size, TokenKind kind);
size_t copy_token_kind_str(char *dest, size_t dest_size, TokenKind kind)
{
	size_t n = 0;
	switch (kind) {
		case TOKEN_EOF:
			n = snprintf(dest, dest_size, "end of file");
			break;
		case TOKEN_INT:
			n = snprintf(dest, dest_size, "%s", "integer");
			break;
		case TOKEN_FLOAT:
			n = snprintf(dest, dest_size, "%s", "float");
			break;
		case TOKEN_NAME:
			n = snprintf(dest, dest_size, "%s", "name");
			break;
		default:
			if (kind < TOKEN_LAST_CHAR + 1 && isprint(kind))
				n = snprintf(dest, dest_size, "%c", kind);
			else
				n = snprintf(dest, dest_size, "<ASCII %d>",
						kind);
			break;	
	}
	return n;
}

/*
 * Warning: This returns a pointer to a static internal buffer, so the next
 * call with overwrite it
 */

const char *token_kind_str(TokenKind kind)
{
	static char buf[256];
	size_t n = copy_token_kind_str(buf, sizeof(buf), kind);
	assert(n + 1 <= sizeof(buf));
	return buf;
}

static uint8_t char_to_digit[256] = {
	['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4, ['5'] = 5,
	['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
	['a'] = 10, ['A'] = 10, ['b'] = 11, ['B'] = 11,
	['c'] = 12, ['C'] = 12, ['d'] = 13, ['D'] = 13,
	['e'] = 14, ['E'] = 14, ['f'] = 15, ['F'] = 15,
};

void scan_int(void) {
	uint64_t base = 10;
	uint64_t val = 0;
	uint64_t digit = 0;
	unsigned index = 0;
	if ('0' == *stream) {
		stream++;
		if (tolower(*stream) == 'x') {
			stream++;
			token.mod = TOKENMOD_HEX;
			base = 16;
		} else if (tolower(*stream) == 'b') {
			stream++;
			token.mod = TOKENMOD_BIN;
			base = 2;
		} else if (isdigit(*stream)) {
			base = 8;
			token.mod = TOKENMOD_OCT;
		} else if (' ' != *stream) {
			syntax_error("Expected hex, bin, oct, received '%c'",
					*stream);
		}
	}
	while(1) {
		index = *stream;
		digit = char_to_digit[index];
		if (0 == digit && '0' != *stream)
			break;
		if (digit >= base) {
			syntax_error("Digit '%c' out of range for base %llu",
					*stream, base);
		}
		if (val > (UINT64_MAX - digit)/base) {
			syntax_error("Integer literal overflow");
			/* below still not good, see if he fixes it!!*/
			index = *stream;
			digit = char_to_digit[index];
			while ( 0 != digit || '0' == *stream) {
				stream++;
				index = *stream;
				digit = char_to_digit[index];
			}
			val = 0;
		}
		val = val * base + digit;
		stream++;
	}

	token.kind = TOKEN_INT;
	token.val_int = val;
}

void scan_float(void)
{
	double val = 0.0;
	const char *start = stream;
	while (isdigit(*stream)) stream++;

	if ('.' == *stream) stream++;
	while (isdigit(*stream)) stream++;

	if (tolower(*stream) == 'e') {
		stream++;
		if ('+' == *stream || '-' == *stream)
			stream++;
		if (!isdigit(*stream))
			syntax_error("Expected digit after float literal "
					"exponent, found '%c'.", *stream);
		while (isdigit(*stream)) stream++;
	}
	val = strtod(start, 0);
	if (HUGE_VAL == val || -HUGE_VAL == val)
		syntax_error("Float literal overflow");

	token.kind = TOKEN_FLOAT;
	token.val_float = val;
}

char escape_to_char[256] = {
	['n'] = '\n',
	['t'] = '\t',
	['r'] = '\r',
	['v'] = '\v',
	['b'] = '\b',
	['a'] = '\a',
	['0'] = '\0',
};

void scan_char(void)
{
	assert('\'' == *stream++);

	char val = '\0'; 
	unsigned index = 0;
	if ('\'' == *stream) {
		syntax_error("Char literal cannot be empty");
		stream++;
	} else if ('\n' == *stream) {
		syntax_error("Char literal cannot contain newline");
		stream++;
	} else if ('\\' == *stream) {
		stream++;
		index = *stream;
		val = escape_to_char[index];
		if ('\0' == val && '0' != *stream) {
			syntax_error("Invalid char literal escape '\\%c'",
					*stream);
		}
		stream++;
	} else {
		val = *stream++;
	}
	if ('\'' != *stream) {
		syntax_error("Expected closing char quote, got '%c'",
				*stream);
	} else {
		stream++;
	}
	token.kind = TOKEN_INT;
	token.val_int = val;
	token.mod = TOKENMOD_CHAR;
}

void scan_str(void)
{
	assert('"' == *stream++);

	unsigned index = 0;
	char val = '\0';
	char *str = 0;
	while (*stream && '"' != *stream) {
		val = *stream;
		if ('\n' == val) {
			syntax_error("String literal cannot contain newline");
		} else if ('\\' == val) {
			stream++;
			index = *stream;
			val = escape_to_char[index];
			if ('\0' == val && '0' != *stream) {
				syntax_error("Invalid string literal escape '\\%c'",
						*stream);
			}
		}
		buf_push(str, val);
		stream++;
	}
	if (*stream) {
		assert('"' == *stream++);
	} else {
		syntax_error("Unexpected end of file within string literal");
	}
	buf_push(str, '\0');
	token.kind = TOKEN_STR;
	token.val_str = str;
}

#define CASE1(c, c1, k1) \
	case c: \
	token.kind = *stream++; \
	if (c1 == *stream) { \
		token.kind = k1; \
		stream++; \
	} \
	break;

#define CASE2(c, c1, k1, c2, k2) \
	case c: \
	token.kind = *stream++; \
	if (c1 == *stream) { \
		token.kind = k1; \
		stream++; \
	} else if (c2 == *stream) { \
		token.kind = k2; \
		stream++; \
	} \
	break;

void next_token()
{
	token.start = stream;
	token.mod = TOKENMOD_NONE;
	switch (*stream) {
		case ' ': case '\n': case '\t': case '\r': case '\v':
			while (isspace(*stream))
				stream++;
			next_token();
			break;
		case '\'':
			scan_char();
			break;
		case '"':
			scan_str();
			break;
		case '.':
			scan_float();
			break;
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			while (isdigit(*stream))
				stream++;
			if ('.' == *stream || 'e' == tolower(*stream)) {
				stream = token.start;
				scan_float();
			} else {
				stream = token.start;
				scan_int();
			}
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case '_':
			while (isalnum(*stream) || '_' == *stream)
				stream++;
			token.kind = TOKEN_NAME;
			token.name = str_intern_range(token.start, stream);
			break;
		case '<':
			token.kind = *stream++;
			if ('<' == *stream) {
				token.kind = TOKEN_LSHIFT;
				stream++;
				if ('=' == *stream) {
					token.kind = TOKEN_ASSIGN_LSHIFT;
					stream++;
				}
			} else if ('=' == *stream) {
				token.kind = TOKEN_LTEQ;
				stream++;
			}
			break;
		case '>':
			token.kind = *stream++;
			if ('>' == *stream) {
				token.kind = TOKEN_RSHIFT;
				stream++;
				if ('=' == *stream) {
					token.kind = TOKEN_ASSIGN_RSHIFT;
					stream++;
				}
			} else if ('=' == *stream) {
				token.kind = TOKEN_GTEQ;
				stream++;
			}
			break;

			CASE1(':', '=', TOKEN_ASSIGN_COLON)
				CASE1('*', '=', TOKEN_ASSIGN_MUL)
				CASE1('/', '=', TOKEN_ASSIGN_DIV)
				CASE1('%', '=', TOKEN_ASSIGN_MOD)
				CASE1('^', '=', TOKEN_ASSIGN_XOR)
				CASE2('+', '+', TOKEN_INC, '=', TOKEN_ASSIGN_ADD)
				CASE2('-', '-', TOKEN_DEC, '=', TOKEN_ASSIGN_SUB)
				CASE2('&', '&', TOKEN_AND, '=', TOKEN_ASSIGN_AND)
				CASE2('|', '|', TOKEN_OR, '=', TOKEN_ASSIGN_OR)

		default:
				token.kind = *stream++;
				break;
	}
	token.end = stream;
}

#undef CASE1
#undef CASE2

void init_stream(const char *str)
{
	stream = str;
	next_token();
}

void print_token(Token token)
{
	switch (token.kind) {
		case TOKEN_INT:
			printf("TOKEN INT: %ld\n", token.val_int);
			break;
		case TOKEN_FLOAT:
			printf("TOKEN FLOAT: %f\n", token.val_float);
			break;
		case TOKEN_NAME:
			printf("TOKEN NAME: %s\n", token.name);
			break;
		default:
			printf("TOKEN '%c'\n", token.kind);	
			break;
	}
}

static inline unsigned is_token(TokenKind kind)
{
	return token.kind == kind;
}

static inline unsigned is_token_name(const char *name)
{
	return token.kind == TOKEN_NAME && token.name == name;
}

/* doesn't error out if there's no match */
static inline unsigned match_token(TokenKind kind)
{
	if (is_token(kind)) {
		next_token();
		return 1;
	} else {
		return 0;
	}
}

static inline unsigned expect_token(TokenKind kind)
{
	if (is_token(kind)) {
		next_token();
		return 1;
	} else {
		char buf[256];
		copy_token_kind_str(buf, sizeof(buf), kind);
		fatal("expected token %s, got %s", buf,
				token_kind_str(token.kind));
		return 0;
	}
}

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) \
		&& match_token(TOKEN_NAME))
#define assert_token_int(x) assert(token.val_int == (x) \
		&& match_token(TOKEN_INT))
#define assert_token_float(x) assert(token.val_float == (x) \
		&& match_token(TOKEN_FLOAT))
#define assert_token_str(x) assert(strcmp(token.val_str, (x)) == 0 \
		&& match_token(TOKEN_STR))
#define assert_token_eof() assert(is_token(TOKEN_EOF))

void test_lex(void)
{
	/* Integer tests */
	init_stream("0 18446744073709551615 0xffffffffffffffff 042 0b1111");
	assert_token_int(0);
	assert_token_int(18446744073709551615ull);
	assert(TOKENMOD_HEX == token.mod);
	assert_token_int(0xffffffffffffffffull);
	assert(TOKENMOD_OCT == token.mod);
	assert_token_int(042);
	assert(TOKENMOD_BIN == token.mod);
	assert_token_int(0xf);
	assert_token_eof();
	init_stream("18446744073709551615 0XFFFFFFFFFFFFFFFF 042 0B1111");
	assert_token_int(18446744073709551615ULL);
	assert_token_int(0XFFFFFFFFFFFFFFFFULL);
	assert_token_int(042);
	assert_token_int(0XF);
	assert_token_eof();

	/* Float tests */
	init_stream("18446744073709551615 0xffffffffffffffff 042 0b1111");
	init_stream("3.14 .123 42. 3e10");
	assert_token_float(3.14);
	assert_token_float(.123);
	assert_token_float(42.);
	assert_token_float(3e10);
	assert_token_eof();

	/* Char tests */
	init_stream("'a' '\\n'");
	assert_token_int('a');
	assert_token_int('\n');
	assert_token_eof();

	/* String tests */
	init_stream("\"foo\" \"a\\nb\"");
	assert_token_str("foo");
	assert_token_str("a\nb");
	assert_token_eof();

	/* Operator tests */
	init_stream(": := + += ++ < <= << <<=");
	assert_token(':');
	assert_token(TOKEN_ASSIGN_COLON);
	assert_token('+');
	assert_token(TOKEN_ASSIGN_ADD);
	assert_token(TOKEN_INC);
	assert_token('<');
	assert_token(TOKEN_LTEQ);
	assert_token(TOKEN_LSHIFT);
	assert_token(TOKEN_ASSIGN_LSHIFT);
	assert_token_eof();

	/* Misc tests */
	init_stream("XY+(XY)_HELLO1,234+994");
	assert_token_name("XY");
	assert_token('+');
	assert_token('(');
	assert_token_name("XY");
	assert_token(')');
	assert_token_name("_HELLO1");
	assert_token(',');
	assert_token_int(234);
	assert_token('+');
	assert_token_int(994);
	assert_token_eof();
}

#undef assert_token
#undef assert_token_name
#undef assert_token_int
#undef assert_token_eof


/*
 * PARSER SECTION (TO DELETE)
 */

/*
 * parser grammar (higher up the ladder has higher precedence):
 *
 * expr3 = INT | '(' expr ')'
 * expr2 = -expr2 | expr3
 * expr1 = expr2 (('*' | '/') expr2)*
 * expr0 = expr1 (('+' | '-') expr1)*
 * expr = expr0
 */

int parse_expr(void);
int parse_expr0(void);
int parse_expr1(void);
int parse_expr2(void);
int parse_expr3(void);

int parse_expr3(void) {
	int val = 0;
	if (is_token(TOKEN_INT)) {
		val = token.val_int;
		next_token();
		return val;
	} else if (match_token('(')) {
		val = parse_expr();
		expect_token(')');
		return val;
	} else {
		fatal("expected integer or '(', got %s",
				token_kind_str(token.kind));
		return 0;
	}
}

int parse_expr2(void)
{
	if (match_token('-')) {
		return -parse_expr2();
	} else {
		return parse_expr3();
	}
}

int parse_expr1(void) {
	char op = '\0';
	int val = parse_expr2();
	int rval = 0;
	while (is_token('*') || is_token('/')) {
		op = token.kind;
		next_token();
		rval = parse_expr2();
		if ('*' == op) {
			val *= rval;
		} else {
			assert('/' == op);
			assert(0 != rval);
			val /= rval;
		}
	}
	return val;
}

int parse_expr0(void) {
	char op = '\0';
	int val = parse_expr1();
	int rval = 0;
	while (is_token('+') || is_token('-')) {
		op = token.kind;
		next_token();
		rval = parse_expr1();
		if ('+' == op) {
			val += rval;
		} else {
			assert('-' == op);
			val -= rval;
		}
	}
	return val;
}

int parse_expr(void) {
	return parse_expr0();
}

int test_parse_expr(const char *expr) {
	init_stream(expr);
	return parse_expr();
}

#define assert_expr(x) assert(test_parse_expr(#x) == (x))
void test_parse(void) {
	assert_expr(1);
	assert_expr(-1);
	assert_expr((1));
	assert_expr((1)*2+7);
	assert_expr(1-2-3);
	assert_expr(2*3+4*5);
	assert_expr(2*(3+4)*5);
	assert_expr(2+-3);
}
#undef TEST_EXPR

/*
 * AST SECTION
 */
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Decl Decl;
typedef struct Typespec Typespec;

typedef enum {
	TYPESPEC_NONE,
	TYPESPEC_NAME,
	TYPESPEC_FUNC,
	TYPESPEC_ARRAY,
	TYPESPEC_POINTER,
} TypespecKind;


typedef struct {
	BUF(Typespec **args);
	Typespec *ret;
} FuncTypespec;

struct Typespec {
	TypespecKind kind;
	struct {
		const char *name;
		FuncTypespec func;
		/* pointer/array types */
		struct {
			Typespec *base;
			Expr *size;
		};
	};
};

typedef enum {
	DECL_NONE,
	DECL_ENUM,
	DECL_STRUCT,
	DECL_UNION,
	DECL_VAR,
	DECL_CONST,
	DECL_TYPEDEF,
	DECL_FUNC,
} DeclKind;

typedef struct {
	const char *name;
	Typespec *type;
} EnumItem;

typedef struct {
	BUF(const char **names);
	Typespec *type;
} AggregateItem;

typedef struct {
	const char *name;
	Typespec *type;
} FuncParam;

typedef struct {
	BUF(FuncParam *params);
	Typespec *ret_type;
} DeclFunc;

struct Decl {
	DeclKind kind;
	const char *name;
	union {
		BUF(EnumItem *enum_items);
		BUF(AggregateItem *aggregate_items);
		struct {
			Typespec *type;
			Expr *expr;
		};
		DeclFunc decl_func;
	};
};

typedef enum {
	EXPR_NONE,
	EXPR_INT,
	EXPR_FLOAT,
	EXPR_STR,
	EXPR_NAME,
	EXPR_CAST,
	EXPR_CALL,
	EXPR_INDEX,
	EXPR_FIELD,
	EXPR_COMPOUND,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_TERNARY,
} ExprKind;

struct Expr {
	ExprKind kind;
	TokenKind op;
	union {
		/* Literals/names */
		uint64_t val_int;
		double val_float;
		const char *val_str;
		const char *name;
		/* Compound Literals */
		struct {
			Typespec *type_compound;
			BUF(Expr **compound_args);
		};
		/* Casts */
		struct {
			Typespec *type_cast;
			BUF(Expr **expr_cast);

		};
		struct {
			/* Unary */
			Expr *operand;
			union {
				BUF(Expr **args);
				Expr *index;
				const char *field;
			};
		};
		struct {
			/* Binary */
			Expr *left;
			Expr *right;
		};
		struct {
			/* Ternary */
			Expr *cond;
			Expr *expr_then;
			Expr *expr_else;
		};
	};
};

Expr *expr_alloc(ExprKind kind)
{
	Expr *expr = calloc(1, sizeof(*expr));
	expr->kind = kind;
	return expr;
}

Expr *expr_int(uint64_t val_int)
{
	Expr *expr = expr_alloc(EXPR_INT);
	expr->val_int = val_int;
	return expr;
}

Expr *expr_float(double val_float)
{
	Expr *expr = expr_alloc(EXPR_FLOAT);
	expr->val_float = val_float;
	return expr;
}

Expr *expr_str(const char *str)
{
	Expr *expr = expr_alloc(EXPR_STR);
	expr->val_str = str;
	return expr;
}

Expr *expr_name(const char *name)
{
	Expr *expr = expr_alloc(EXPR_NAME);
	expr->name = name;
	return expr;
}

Expr *expr_cast(Typespec *type, Expr *expr)
{
	Expr *new_expr = expr_alloc(EXPR_CAST);
	new_expr->type_cast = type;
	/* NEEDS FIXING BELOW, INCONPATIBLE TYPES */
	printf("line %d needs fixing\n", __LINE__ + 1);
	new_expr->expr_cast = expr;
	return new_expr;
}

Expr *expr_call(Expr *operand, Expr **args)
{
	Expr *expr = expr_alloc(EXPR_CALL);
	expr->operand = operand;
	expr->args = args;
	return expr;
}

Expr *expr_index(Expr *operand, Expr *index)
{
	Expr *expr = expr_alloc(EXPR_INDEX);
	expr->operand = operand;
	expr->index = index;
	return expr;
}

Expr *expr_field(Expr *operand, const char *field)
{
	Expr *expr = expr_alloc(EXPR_FIELD);
	expr->operand = operand;
	expr->field = field;
	return expr;
}

Expr *expr_unary(TokenKind op, Expr *operand)
{
	Expr *new_expr = expr_alloc(EXPR_UNARY);
	new_expr->op = op;
	new_expr->operand = operand;
	return new_expr;
}

Expr *expr_binary(TokenKind op, Expr *left, Expr *right)
{
	Expr *expr = expr_alloc(EXPR_BINARY);
	expr->op = op;
	expr->left = left;
	expr->right = right;
	return expr;
}

Expr *expr_ternary(Expr *cond, Expr *expr_then, Expr *expr_else)
{
	Expr *expr = expr_alloc(EXPR_TERNARY);
	expr->cond = cond;
	expr->expr_then = expr_then;
	expr->expr_else = expr_else;
	return expr;
}

void expr_print(Expr *expr);
void print_type(Typespec *type)
{
	FuncTypespec func = (FuncTypespec){0};
	switch(type->kind) {
		case TYPESPEC_NONE:
			break;
		case TYPESPEC_NAME:
			printf("%s", type->name);
			break;
		case TYPESPEC_FUNC:
			func = type->func;
			printf("(func ");
			for (Typespec **it = func.args;
					it != buf_end(func.args); it++) {
				printf(" ");
				print_type(*it);
			}
			printf(") ");
			print_type(func.ret);
			printf(")");
			break;
		case TYPESPEC_ARRAY:
			printf("(array ");
			print_type(type->base);
			printf(" ");
			expr_print(type->size);
			printf(")");
			break;
		case TYPESPEC_POINTER:
			printf("(pointer ");
			print_type(type->base);
			printf(")");
			break;
		default: 
			assert(0);
	}
}

void expr_print(Expr *expr)
{
	switch (expr->kind) {
		case EXPR_NONE:
			break;
		case EXPR_INT:
			printf("%lu", expr->val_int);
			break;
		case EXPR_FLOAT:
			printf("%f", expr->val_float);
			break;
		case EXPR_STR:
			printf("\"%s\"", expr->val_str);
			break;
		case EXPR_NAME:
			printf("%s", expr->name);
			break;
		case EXPR_CAST:
			printf("(cast ");
			print_type(expr->type_cast);
			printf(" ");
			expr_print(expr->expr_cast);
			printf(")");
			break;
		case EXPR_CALL:
			printf("(");
			expr_print(expr->operand);
			for (Expr **it = expr->args;
					it != buf_end(expr->args); it++) {
				printf(" ");
				expr_print(*it);
			}
			printf(")");
			break;
		case EXPR_INDEX:
			printf("(index ");
			expr_print(expr->operand);
			printf(" ");
			expr_print(expr->index);
			printf(")");
			break;
		case EXPR_FIELD:
			printf("(field ");
			expr_print(expr->operand);
			printf(" %s)", expr->field);
			break;
		case EXPR_COMPOUND:
			printf("(compound ...)");
			break;
		case EXPR_UNARY:
			printf("(%c ", expr->op);
			expr_print(expr->operand);
			printf(")");
			break;
		case EXPR_BINARY:
			printf("(%c ", expr->op);
			expr_print(expr->left);
			printf(" ");
			expr_print(expr->right);
			printf(")");
			break;
		case EXPR_TERNARY:
			printf("(ternary ");
			expr_print(expr->cond);
			printf(" ");
			expr_print(expr->expr_then);
			printf(" ");
			expr_print(expr->expr_else);
			printf(")");
			break;
		default:
			assert(0);
	}
}

typedef enum {
	STMT_NONE,
	STMT_RETURN,
	STMT_BREAK,
	STMT_CONTINUE,
	STMT_BLOCK,
	STMT_IF,
	STMT_WHILE,
	STMT_FOR,
	STMT_DO,
	STMT_SWITCH,
	STMT_ASSIGN,
	STMT_AUTO_ASSIGN,
	STMT_EXPR,

} StmtKind;

typedef struct {
	BUF(Stmt **stmts);
} StmtBlock;

typedef struct {
	Expr *cond;
	StmtBlock block;
} ElseIf;

typedef struct {
	BUF(Expr **exprs);
	StmtBlock block;
} Case;

struct Stmt {
	StmtKind kind;
	Expr *expr;
	StmtBlock block;
	union {
		/* if */
		struct {
			BUF(ElseIf *elseifs);
			StmtBlock block_else;
		};
		/* for */
		struct {
			StmtBlock for_init;
			StmtBlock for_next;
		};
		/* switch */
		struct {
			BUF(Case *cases);
		};
		/* Auto-assign */
		struct {
			const char *name;
		};
		/* Assignment operators */
		struct {
			Expr *rhs;
		};
	};
};

void expr_println(Expr *expr)
{
	expr_print(expr);
	printf("\n");
}

void test_expr(void)
{
	Expr *exprs[] = {
		expr_unary('-', expr_float(3.14)),
		expr_binary('+', expr_int(1), expr_int(2)),
		expr_ternary(expr_name("flag"), expr_str("true"),
				expr_str("false")),
		expr_field(expr_name("person"), "name"),

	};
	for (Expr **it = exprs; it != exprs + sizeof(exprs)/sizeof(exprs[0]);
			it++)
		expr_println(*it);
}

void test_ast(void)
{
	test_expr();
}

void run_tests(void)
{
	test_buf();	
	test_lex();
	test_str_intern();
	test_parse();
	test_ast();
}

int main(void)
{
	run_tests();
	return 0;
}
