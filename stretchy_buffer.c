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
#define buf_push(b, x) (buf__fit((b), 1), (b)[buf__hdr(b)->len++] = (x))
#define buf_free(b) ((b) ? free(buf__hdr(b)), (b) = 0 : 0)

void test_buf(void);
void test_str_intern(void);
void test_lex(void);
void *buf__grow(const void *buf, size_t new_len, size_t elem_size);

const char *str_intern_range(const char *start, const char *end);
const char *str_intern(const char *str);

void next_token();

void *buf__grow(const void *buf, size_t new_len, size_t elem_size)
{
	size_t new_cap = MAX(1 + 2 * buf_cap(buf), new_len);
	assert(new_len <= new_cap);
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
} InternStr;

static InternStr *interns;

const char *str_intern_range(const char *start, const char *end)
{
	size_t len = end - start;
	for (size_t i = 0; i < buf_len(interns); i++) {
		if (interns[i].len == len
				&& strncmp(interns[i].str, start, len) == 0) {
			return interns[i].str;
		}
	}
	char *str = malloc(len + 1);
	memcpy(str, start, len);
	str[len] = '\0';
	buf_push(interns, ((InternStr){len, str}));
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
	};
} Token;

static Token token;
static const char *stream;

void next_token()
{
	int val = 0;
	token.start = stream;
	switch (*stream) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			while(isdigit(*stream)) {
				val *= 10;
				val += *stream++ - '0';
			}
			token.kind = TOKEN_INT;
			token.val = val;
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case '_':
			while (isalnum(*stream) || '_' == *stream)
				stream++;
			token.kind = TOKEN_NAME;
			break;
		default:
			token.kind = *stream++;
			break;
	}
	token.end = stream;
}

void print_token(Token token)
{
	int len = 0;
	switch (token.kind) {
		case TOKEN_INT:
			printf("TOKEN INT: %d\n", token.val);
			break;
		case TOKEN_NAME:
			len = token.end - token.start;
			printf("TOKEN NAME: %.*s\n", len, token.start);
			break;
		default:
			printf("TOKEN '%c'\n", token.kind);	
			break;
	}
}

void test_lex(void)
{
	char *source = "+()_HELLO1,234+FOO!994";	
	stream = source;
	next_token();
	while (token.kind) {
		print_token(token);
		next_token();
	}
}

int main(void)
{
	test_buf();	
	test_lex();
	test_str_intern();
	return 0;
}
