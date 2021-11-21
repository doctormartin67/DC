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
void test_buf(void);
void test_str_intern(void);
void test_lex(void);
void *buf__grow(const void *buf, size_t new_len, size_t elem_size);

const char *str_intern_range(const char *start, const char *end);
const char *str_intern(const char *str);

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
	TOKEN_NAME,
} TokenKind;

typedef struct {
	TokenKind kind;
	const char *start;
	const char *end;
	union {
		int val;
		const char *name;
	};
} Token;

static Token token;
static const char *stream;

const char *keyword_if;
const char *keyword_for;
const char *keyword_while;

void init_keywords() {
	keyword_if = str_intern("if");
	keyword_for = str_intern("for");
	keyword_while = str_intern("while");
}

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

void next_token()
{
	int val = 0;
	token.start = stream;
	switch (*stream) {
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			while(isdigit(*stream)) {
				val *= 10;
				val += *stream++ - '0';
			}
			token.kind = TOKEN_INT;
			token.val = val;
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
		default:
			token.kind = *stream++;
			break;
	}
	token.end = stream;
}

void init_stream(const char *str)
{
	stream = str;
	next_token();
}

void print_token(Token token)
{
	switch (token.kind) {
		case TOKEN_INT:
			printf("TOKEN INT: %d\n", token.val);
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
#define assert_token_int(x) assert(token.val == (x) && match_token(TOKEN_INT))
#define assert_token_eof() assert(is_token(TOKEN_EOF))

void test_lex(void)
{
	const char *str = "XY+(XY)_HELLO1,234+994";	
	init_stream(str);
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
 * PARSER SECTION
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
		val = token.val;
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

void run_tests(void)
{
	test_buf();	
	test_lex();
	test_str_intern();
	test_parse();
}

int main(void)
{
	run_tests();
	return 0;
}
