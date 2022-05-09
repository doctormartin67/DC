#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h> // defines ULLONG
#include <math.h> // defines HUGE_VAL
#include <ctype.h>
#include "lex.h"
#include "common.h"

static SrcPos pos_builtin = {.name = "<builtin>"};
static Token token;
static const char *code;
static const char *stream;
static const char *line_start;

#define KEYWORD(name) name##_keyword = str_intern(#name);

void init_keywords(void)
{
	static unsigned inited;
	if (inited) {
		return;
	}
	KEYWORD(dim);
	const char *arena_end = intern_arena_end();
	KEYWORD(as);
	KEYWORD(if);
	KEYWORD(then);
	KEYWORD(else);
	KEYWORD(elseif);
	KEYWORD(end);

	KEYWORD(mod);
	KEYWORD(true);
	KEYWORD(false);
	KEYWORD(and);
	KEYWORD(or);
	KEYWORD(xor);

	KEYWORD(while);
	KEYWORD(until);
	KEYWORD(wend);
	KEYWORD(do);
	KEYWORD(loop);
	KEYWORD(for);
	KEYWORD(to);
	KEYWORD(step);
	KEYWORD(next);
	KEYWORD(select);
	KEYWORD(case);
	KEYWORD(project);
	KEYWORD(is);
	assert(intern_arena_end() == arena_end);
	first_keyword = dim_keyword;
	last_keyword = is_keyword;

	first_op_keyword = mod_keyword;
	last_op_keyword = xor_keyword;

	inited = 1;
}

#undef KEYWORD

unsigned is_keyword_name(const char *name)
{
	assert(first_keyword && last_keyword);
	return first_keyword <= name && name <= last_keyword;
}

static const char *const token_kind_names[] = {
	[TOKEN_EOF] = "EOF",
	[TOKEN_COLON] = ":",
	[TOKEN_LPAREN] = "(",
	[TOKEN_RPAREN] = ")",
	[TOKEN_LBRACKET] = "[",
	[TOKEN_RBRACKET] = "]",
	[TOKEN_COMMA] = ",",
	[TOKEN_DOT] = ".",
	[TOKEN_POUND] = "#",
	[TOKEN_QUESTION] = "?",
	[TOKEN_KEYWORD] = "keyword",
	[TOKEN_INT] = "int",
	[TOKEN_FLOAT] = "float",
	[TOKEN_STR] = "string",
	[TOKEN_NAME] = "name",
	[TOKEN_NEG] = "~",
	[TOKEN_NOT] = "!",
	[TOKEN_MUL] = "*",
	[TOKEN_DIV] = "/",
	[TOKEN_MOD] = "Mod",
	[TOKEN_AND] = "&",
	[TOKEN_ADD] = "+",
	[TOKEN_SUB] = "-",
	[TOKEN_EXP] = "^",
	[TOKEN_NOTEQ] = "<>",
	[TOKEN_LT] = "<",
	[TOKEN_GT] = ">",
	[TOKEN_LTEQ] = "<=",
	[TOKEN_GTEQ] = ">=",
	[TOKEN_AND_AND] = "And",
	[TOKEN_OR_OR] = "Or",
	[TOKEN_XOR_XOR] = "Xor",
	[TOKEN_ASSIGN] = "=",
};

const char *token_kind_name(TokenKind kind)
{
	if (kind < sizeof(token_kind_names)/sizeof(*token_kind_names)) {
		return token_kind_names[kind];
	} else {
		return "<unknown>";
	}
}

void warning(SrcPos pos, const char *fmt, ...)
{
	if (pos.name == 0) {
		pos = pos_builtin;
	}
	va_list args;
	va_start(args, fmt);
	printf("%s(%d): warning: ", pos.name, pos.line);
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}

void syntax_error(SrcPos pos, const char *fmt, ...)
{
	if (pos.name == 0) {
		pos = pos_builtin;
	}
	char buf[BUFSIZ];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	buf_printf(error.str, "syntax error at: %s(%d)\n", pos.name, pos.line);
	buf_printf(error.str, "%s", buf);
	va_end(args);
	error.is_error = 1;
	longjmp(error.buf, 1);
}

const char *token_info(void)
{
	if (token.kind == TOKEN_NAME || token.kind == TOKEN_KEYWORD) {
		return token.name;
	} else {
		return token_kind_name(token.kind);
	}
}

static uint8_t char_to_digit[256] = {
	['0'] = 0,
	['1'] = 1,
	['2'] = 2,
	['3'] = 3,
	['4'] = 4,
	['5'] = 5,
	['6'] = 6,
	['7'] = 7,
	['8'] = 8,
	['9'] = 9,
};

static void scan_int(void);
static void scan_int(void)
{
	unsigned base = 10;
	unsigned digit = 0;
	unsigned long long val = 0;
	uint8_t index = 0;
	const char *start_digits = stream;
	if ('0' == *stream) {
		stream++;
	}
	while (1) {
		index = *stream;
		digit = char_to_digit[index];
		if (0 == digit && '0' != *stream) {
			break;
		}
		if (val > (ULLONG_MAX - digit)/base) {
			error_here("Integer literal overflow");
			while (isdigit(*stream)) {
				stream++;
			}
			val = 0;
			break;
		}
		val = val*base + digit;
		stream++;
	}
	if (stream == start_digits) {
		error_here("Expected base %d digit, got '%c'", base, *stream);
	}
	token.kind = TOKEN_INT;
	token.int_val = val;
}

static void scan_float(void) {
	const char *start = stream;
	double val = 0.0;

	while (isdigit(*stream)) stream++;
	if ('.' == *stream) stream++;
	while (isdigit(*stream)) stream++;

	val = strtod(start, 0);
	if (val == HUGE_VAL) {
		error_here("Float literal overflow");
	}
	token.kind = TOKEN_FLOAT;
	token.float_val = val;
}

/*
 * 'strs' is used to clear all strings once we are done with a stream
 */
static char **strs;
static void scan_str(void) {
	assert('"' == *stream);
	stream++;
	char *str = 0;
	char val = '\0';
	while (*stream && '"' != *stream) {
		val = *stream;
		if (val == '\n') {
			error_here("String literal cannot contain newline");
			break;
		} else {
			stream++;
		}
		buf_push(str, val);
	}
	if (*stream) {
		stream++;
	} else {
		error_here("Unexpected end of file within string literal");
	}
	buf_push(str, '\0');
	token.kind = TOKEN_STR;
	token.str_val = str;
	buf_push(strs, str);
}

#define CASE1(c1, k1) \
	case c1: \
	token.kind = k1; \
	stream++; \
	break;

#define CASE2(c1, k1, c2, k2) \
	case c1: \
	token.kind = k1; \
	stream++; \
	if (*stream == c2) { \
		token.kind = k2; \
		stream++; \
	} \
	break;

#define CASE3(c1, k1, c2, k2, c3, k3) \
	case c1: \
	token.kind = k1; \
	stream++; \
	if (*stream == c2) { \
		token.kind = k2; \
		stream++; \
	} else if (*stream == c3) { \
		token.kind = k3; \
		stream++; \
	} \
	break;

void next_token(void)
{
	char c = '\0';
repeat:
	token.start = stream;
	switch (*stream) {
		case ' ': case '\n': case '\r': case '\t': case '\v':
			while (isspace(*stream)) {
				if (*stream++ == '\n') {
					line_start = stream;
					token.pos.line++;
				}
			}
			goto repeat;
		case '"':
			scan_str();
			break;
		case '.':
			if (isdigit(stream[1])) {
				scan_float();
			} else {
				stream++;
			}
			break;
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			while (isdigit(*stream)) {
				stream++;
			}
			c = *stream;
			stream = token.start;
			if (c == '.') {
				scan_float();
			} else {
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
		case 'Y': case 'Z': case '_':
			while (isalnum(*stream) || *stream == '_') {
				stream++;
			}
			token.name = str_intern_range(token.start, stream);
			token.kind = is_keyword_name(token.name)
					? TOKEN_KEYWORD : TOKEN_NAME;
			break;
		case '\'':
			stream++;
			while (*stream && '\n' != *stream) {
				stream++;
			}
			goto repeat;
			break;
			CASE1('\0', TOKEN_EOF);
			CASE1('(', TOKEN_LPAREN);
			CASE1(')', TOKEN_RPAREN);
			CASE1('[', TOKEN_LBRACKET);
			CASE1(']', TOKEN_RBRACKET);
			CASE1(',', TOKEN_COMMA);
			CASE1('#', TOKEN_POUND);
			CASE1('?', TOKEN_QUESTION);
			CASE1('~', TOKEN_NEG);
			CASE1('!', TOKEN_NOT);
			CASE1(':', TOKEN_COLON);
			CASE1('=', TOKEN_ASSIGN);
			CASE1('^', TOKEN_EXP);
			CASE1('*', TOKEN_MUL);
			CASE1('/', TOKEN_DIV);
			CASE1('%', TOKEN_MOD);
			CASE1('+', TOKEN_ADD);
			CASE1('-', TOKEN_SUB);
			CASE1('&', TOKEN_AND);
			CASE2('>', TOKEN_GT, '=', TOKEN_GTEQ);
			CASE3('<', TOKEN_LT, '=', TOKEN_LTEQ, '>', TOKEN_NOTEQ);
		default:
			error_here("Invalid '%c' token, skipping", *stream);
			stream++;
			goto repeat;
	}
	token.end = stream;
}

#undef CASE1
#undef CASE2
#undef CASE3

void init_stream(const char *name, const char *buf)
{
	code = buf;
	stream = buf;
	line_start = stream;
	token.pos.name = name ? name : "<editor>";
	token.pos.line = 1;
	next_token();
}

void clear_stream(void)
{
	for (size_t i = 0; i < buf_len(strs); i++) {
		buf_free(strs[i]);
	}
	buf_free(strs);
}

unsigned is_token(TokenKind kind)
{
	return token.kind == kind;
}

unsigned is_token_eof(void)
{
	return token.kind == TOKEN_EOF;
}

unsigned is_a_keyword(const char *name)
{
	return is_token(TOKEN_KEYWORD) && token.name == name;
}

unsigned match_keyword(const char *name)
{
	if (is_a_keyword(name)) {
		next_token();
		return 1;
	} else {
		return 0;
	}
}

unsigned expect_keyword(const char *name)
{
	if (is_a_keyword(name)) {
		next_token();
		return 1;
	} else {
		if (TOKEN_EOF == token.kind) {
			error_here("Expected keyword '%s', but reached "
					"end of file", name);
		} else {
			error_here("Expected keyword '%s', got '%s'",
					name, token.name);
		}
		return 0;
	}
}

unsigned match_token(TokenKind kind)
{
	if (is_token(kind)) {
		next_token();
		return 1;
	} else {
		return 0;
	}
}

unsigned expect_token(TokenKind kind)
{
	if (is_token(kind)) {
		next_token();
		return 1;
	} else {
		error_here("Expected token %s, got %s",
				token_kind_name(kind), token_info());
		return 0;
	}
}

SrcPos token_pos(void)
{
	return token.pos;
}

const char *token_name(void)
{
	return token.name;
}

TokenKind token_kind(void)
{
	return token.kind;
}

unsigned long long token_int_val(void)
{
	return token.int_val;
}

const char *token_start(void)
{
	return token.start;
}

const char *token_end(void)
{
	return token.end;
}

double token_float_val(void)
{
	return token.float_val;
}

const char *token_str_val(void)
{
	return token.str_val;
}
