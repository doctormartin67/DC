/*
 * one of the most useful tricks in C ever invented.
 * push and pop element in an array of arbitrary type
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h> /*defines HUGE_VAL */

#define TODO(x)

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)
#define IS_POW2(x) (((x) != 0) && ((x) & ((x)-1)) == 0)
#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))
#define ALIGN_DOWN_PTR(p, a) ((void *)ALIGN_DOWN((uintptr_t)(p), (a)))
#define ALIGN_UP_PTR(p, a) ((void *)ALIGN_UP((uintptr_t)(p), (a)))

typedef struct BufHdr {
	size_t len;
	size_t cap;
	char buf[0];
} BufHdr;

/*
 * convention: single underscores are for public use, double underscores are
 * for private internal use
 */

#define buf__hdr(b) ((BufHdr *)((char *)(b) - offsetof(BufHdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b)*sizeof(*b) : 0)

#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = 0) : 0)
#define buf_fit(b, n) ((n) <= buf_cap(b) ? 0 \
		: ((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...) (buf_fit((b), 1 + buf_len(b)), \
		(b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_printf(b, ...) ((b) = buf__printf((b), __VA_ARGS__))
#define buf_clear(b) ((b) ? buf__hdr(b)->len = 0 : 0)

void *buf__grow(const void *buf, size_t new_len, size_t elem_size);
char *buf__printf(char *buf, const char *fmt, ...);

void *buf__grow(const void *buf, size_t new_len, size_t elem_size)
{
	assert(buf_cap(buf) <= (SIZE_MAX - 1)/2);
	size_t new_cap = CLAMP_MIN(2*buf_cap(buf), MAX(new_len, 16));
	assert(new_len <= new_cap);
	assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf))/elem_size);
	size_t new_size = offsetof(BufHdr, buf) + new_cap*elem_size;
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

char *buf__printf(char *buf, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	size_t cap = buf_cap(buf) - buf_len(buf);
	size_t n = 1 + vsnprintf(buf_end(buf), cap, fmt, args);
	va_end(args);
	if (n > cap) {
		buf_fit(buf, n + buf_len(buf));
		va_start(args, fmt);
		cap = buf_cap(buf) - buf_len(buf);
		n = 1 + vsnprintf(buf_end(buf), cap, fmt, args);
		assert(n <= cap);
		va_end(args);
	}
	buf__hdr(buf)->len += n - 1;
	return buf;
}

void test_buf(void)
{
	unsigned *buf = 0;
	assert(buf_len(buf) == 0);
	unsigned n = 1024;
	for (unsigned i = 0; i < n; i++) {
		buf_push(buf, i);
	}
	assert(buf_len(buf) == n);
	for (size_t i = 0; i < buf_len(buf); i++) {
		assert(buf[i] == i);
	}
	buf_free(buf);
	assert(buf == 0);
	assert(buf_len(buf) == 0);
	char *str = 0;
	buf_printf(str, "One: %d\n", 1);
	assert(strcmp(str, "One: 1\n") == 0);
	buf_printf(str, "Hex: 0x%x\n", 0x12345678);
	assert(strcmp(str, "One: 1\nHex: 0x12345678\n") == 0);
}

// Arena allocator

typedef struct Arena {
	char *ptr;
	char *end;
	char **blocks;
} Arena;

#define ARENA_ALIGNMENT 8
#define ARENA_BLOCK_SIZE (1024 * 1024)

void arena_grow(Arena *arena, size_t min_size)
{
	size_t size = ALIGN_UP(CLAMP_MIN(min_size, ARENA_BLOCK_SIZE),
			ARENA_ALIGNMENT);
	arena->ptr = malloc(size);
	assert(arena->ptr == ALIGN_DOWN_PTR(arena->ptr, ARENA_ALIGNMENT));
	arena->end = arena->ptr + size;
	buf_push(arena->blocks, arena->ptr);
}

void *arena_alloc(Arena *arena, size_t size)
{
	void *ptr = 0;
	if (0 == arena->end || 0 == arena->ptr
			|| (size > (arena->end - arena->ptr))) {
		arena_grow(arena, size);
		assert(size <= (arena->end - arena->ptr));
	}
	ptr = arena->ptr;
	arena->ptr = ALIGN_UP_PTR(arena->ptr + size, ARENA_ALIGNMENT);
	assert(arena->ptr <= arena->end);
	assert(ptr == ALIGN_DOWN_PTR(ptr, ARENA_ALIGNMENT));
	return ptr;
}

void arena_free(Arena *arena)
{
	for (char **it = arena->blocks; it != buf_end(arena->blocks); it++)
		free(*it);
	buf_free(arena->blocks);
}

static Arena arena;
void test_arena(void)
{
	int *ptr = 0;
	ptr = arena_alloc(&arena, 64);
	*ptr = 4;
	assert(4 == *ptr);
	assert(ARENA_BLOCK_SIZE == arena.end - arena.ptr + 64);
	assert(arena.ptr - (char *)ptr == 64);
	assert(*arena.blocks == (char *)ptr);

	double *ptrd = 0;
	ptrd = arena_alloc(&arena, 256);
	*ptrd = 0.32;
	assert(0.32 == *ptrd);
	assert(ARENA_BLOCK_SIZE == arena.end - arena.ptr + 64 + 256);
	assert(arena.ptr - (char *)ptrd == 256);
	assert(buf_len(arena.blocks) == 1);

	arena_free(&arena);	
}

// Hash map

uint64_t hash_uint64(uint64_t x) {
	x *= 0xff51afd7ed558ccd;
	x ^= x >> 32;
	return x;
}

uint64_t hash_ptr(const void *ptr) {
	return hash_uint64((uintptr_t)ptr);
}

uint64_t hash_mix(uint64_t x, uint64_t y) {
	x ^= y;
	x *= 0xff51afd7ed558ccd;
	x ^= x >> 32;
	return x;
}

uint64_t hash_bytes(const void *ptr, size_t len) {
	uint64_t x = 0xcbf29ce484222325;
	const char *buf = (const char *)ptr;
	for (size_t i = 0; i < len; i++) {
		x ^= buf[i];
		x *= 0x100000001b3;
		x ^= x >> 32;
	}
	return x;
}

typedef struct Map {
	uint64_t *keys;
	uint64_t *vals;
	size_t len;
	size_t cap;
} Map;

uint64_t map_get_uint64_from_uint64(Map *map, uint64_t key) {
	if (map->len == 0) {
		return 0;
	}
	assert(IS_POW2(map->cap));
	size_t i = hash_uint64(key);
	assert(map->len < map->cap);
	while (1) {
		i &= map->cap - 1;
		if (map->keys[i] == key) {
			return map->vals[i];
		} else if (!map->keys[i]) {
			return 0;
		}
		i++;
	}
	return 0;
}

void map_put_uint64_from_uint64(Map *map, uint64_t key, uint64_t val);

void map_grow(Map *map, size_t new_cap) {
	new_cap = CLAMP_MIN(new_cap, 16);
	Map new_map = {
		.keys = calloc(new_cap, sizeof(uint64_t)),
		.vals = malloc(new_cap * sizeof(uint64_t)),
		.cap = new_cap,
	};
	for (size_t i = 0; i < map->cap; i++) {
		if (map->keys[i]) {
			map_put_uint64_from_uint64(&new_map, map->keys[i], map->vals[i]);
		}
	}
	free(map->keys);
	free(map->vals);
	*map = new_map;
}

void map_put_uint64_from_uint64(Map *map, uint64_t key, uint64_t val) {
	assert(key);
	if (!val) {
		return;
	}
	if (2*map->len >= map->cap) {
		map_grow(map, 2*map->cap);
	}
	assert(2*map->len < map->cap);
	assert(IS_POW2(map->cap));
	size_t i = hash_uint64(key);
	while (1) {
		i &= map->cap - 1;
		if (!map->keys[i]) {
			map->len++;
			map->keys[i] = key;
			map->vals[i] = val;
			return;
		} else if (map->keys[i] == key) {
			map->vals[i] = val;
			return;
		}
		i++;
	}
}

void *map_get(Map *map, const void *key) {
	return (void *)(uintptr_t)map_get_uint64_from_uint64(map, (uint64_t)(uintptr_t)key);
}

void map_put(Map *map, const void *key, void *val) {
	map_put_uint64_from_uint64(map, (uint64_t)(uintptr_t)key, (uint64_t)(uintptr_t)val);
}

void *map_get_from_uint64(Map *map, uint64_t key) {
	return (void *)(uintptr_t)map_get_uint64_from_uint64(map, key);
}

void map_put_from_uint64(Map *map, uint64_t key, void *val) {
	map_put_uint64_from_uint64(map, key, (uint64_t)(uintptr_t)val);
}

uint64_t map_get_uint64(Map *map, void *key) {
	return map_get_uint64_from_uint64(map, (uint64_t)(uintptr_t)key);
}

void map_put_uint64(Map *map, void *key, uint64_t val) {
	map_put_uint64_from_uint64(map, (uint64_t)(uintptr_t)key, val);
}

void test_map(void) {
	Map map = {0};
	enum { N = 1024 };
	for (size_t i = 1; i < N; i++) {
		map_put(&map, (void *)i, (void *)(i+1));
	}
	for (size_t i = 1; i < N; i++) {
		void *val = map_get(&map, (void *)i);
		assert(val == (void *)(i+1));
	}
}

/* string interning */

typedef struct Intern {
	size_t len;
	struct Intern *next;
	char str[0];
} Intern;

static Arena intern_arena;
static Map interns;
static size_t intern_memory_usage;

const char *str_intern_range(const char *start, const char *end)
{
	size_t len = end - start;
	uint64_t hash = hash_bytes(start, len);
	uint64_t key = hash ? hash : 1;
	Intern *intern = map_get_from_uint64(&interns, key);
	for (Intern *it = intern; it; it = it->next) {
		if (it->len == len && strncmp(it->str, start, len) == 0) {
			return it->str;
		}
	}
	Intern *new_intern = arena_alloc(&intern_arena, offsetof(Intern, str) + len + 1);
	new_intern->len = len;
	new_intern->next = intern;
	memcpy(new_intern->str, start, len);
	new_intern->str[len] = '\0';
	map_put_from_uint64(&interns, key, new_intern);
	intern_memory_usage += sizeof(Intern) + len + 1 + 16; /* 16 is estimate of hash table cost */
	return new_intern->str;
}

const char *str_intern(const char *str)
{
	return str_intern_range(str, str + strlen(str));
}

unsigned str_islower(const char *str)
{
	while (*str) {
		if (isalpha(*str) && !islower(*str)) {
			return 0;
		}
		str++;
	}
	return 1;
}

void test_str_intern(void)
{
	const char x[] = "hello";
	const char y[] = "hello";
	const char z[] = "hello!";
	const char a[] = "shello";
	assert(x != y);
	assert(str_intern(x) == str_intern(y));
	assert(str_intern(x) != str_intern(z));
	assert(str_intern(y) != str_intern(a));
	assert(str_intern(z) != str_intern(a));
}

/* lexing */

typedef enum TokenKind {
	TOKEN_EOF,
	TOKEN_COLON,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_AT,
	TOKEN_POUND,
	TOKEN_QUESTION,
	TOKEN_SEMICOLON,
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
	TOKEN_MOD,
	TOKEN_AND,
	TOKEN_LAST_MUL = TOKEN_AND,
	// Additive precedence
	TOKEN_FIRST_ADD,
	TOKEN_ADD = TOKEN_FIRST_ADD,
	TOKEN_SUB,
	TOKEN_XOR,
	TOKEN_OR,
	TOKEN_LAST_ADD = TOKEN_OR,
	// Comparative precedence
	TOKEN_FIRST_CMP,
	TOKEN_EQ = TOKEN_FIRST_CMP,
	TOKEN_NOTEQ,
	TOKEN_LT,
	TOKEN_GT,
	TOKEN_LTEQ,
	TOKEN_GTEQ,
	TOKEN_LAST_CMP = TOKEN_GTEQ,
	TOKEN_AND_AND,
	TOKEN_OR_OR,
	// Assignment operators
	TOKEN_FIRST_ASSIGN,
	TOKEN_ASSIGN = TOKEN_FIRST_ASSIGN,
	TOKEN_LAST_ASSIGN = TOKEN_ASSIGN,
	NUM_TOKEN_KINDS,
} TokenKind;

const char *const token_kind_names[] = {
	[TOKEN_EOF] = "EOF",
	[TOKEN_COLON] = ":",
	[TOKEN_LPAREN] = "(",
	[TOKEN_RPAREN] = ")",
	[TOKEN_LBRACE] = "{",
	[TOKEN_RBRACE] = "}",
	[TOKEN_LBRACKET] = "[",
	[TOKEN_RBRACKET] = "]",
	[TOKEN_COMMA] = ",",
	[TOKEN_DOT] = ".",
	[TOKEN_AT] = "@",
	[TOKEN_POUND] = "#",
	[TOKEN_QUESTION] = "?",
	[TOKEN_SEMICOLON] = ";",
	[TOKEN_KEYWORD] = "keyword",
	[TOKEN_INT] = "int",
	[TOKEN_FLOAT] = "float",
	[TOKEN_STR] = "string",
	[TOKEN_NAME] = "name",
	[TOKEN_NEG] = "~",
	[TOKEN_NOT] = "!",
	[TOKEN_MUL] = "*",
	[TOKEN_DIV] = "/",
	[TOKEN_MOD] = "%",
	[TOKEN_AND] = "&",
	[TOKEN_ADD] = "+",
	[TOKEN_SUB] = "-",
	[TOKEN_OR] = "|",
	[TOKEN_XOR] = "^",
	[TOKEN_EQ] = "==",
	[TOKEN_NOTEQ] = "!=",
	[TOKEN_LT] = "<",
	[TOKEN_GT] = ">",
	[TOKEN_LTEQ] = "<=",
	[TOKEN_GTEQ] = ">=",
	[TOKEN_AND_AND] = "&&",
	[TOKEN_OR_OR] = "||",
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

typedef struct SrcPos {
	const char *name;
	int line;
} SrcPos;

static SrcPos pos_builtin = {.name = "<builtin>"};

typedef struct Token {
	TokenKind kind;
	TODO(TokenSuffix suffix);
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

static Token token;
static const char *stream;

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

void error(SrcPos pos, const char *fmt, ...)
{
	if (pos.name == 0) {
		pos = pos_builtin;
	}
	va_list args;
	va_start(args, fmt);
	printf("%s(%d): error: ", pos.name, pos.line);
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}

#define fatal_error(...) (error(__VA_ARGS__), exit(1))
#define error_here(...) (error(token.pos, __VA_ARGS__))
#define warning_here(...) (error(token.pos, __VA_ARGS__))
#define fatal_error_here(...) (error_here(__VA_ARGS__), exit(1)) // should be abort()

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

void scan_float(void) {
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

void scan_str(void) {
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
	TODO(token.mod = 0);
	TODO(token.suffix = 0);
	switch (*stream) {
		case ' ': case '\n': case '\r': case '\t': case '\v':
			while (isspace(*stream)) {
				if (*stream++ == '\n') {
					TODO(line_start = stream);
					TODO(token.pos.line++);
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
				TODO(token.kind = TOKEN_DOT);
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
			TODO(token.kind = is_keyword_name(token.name)
					? TOKEN_KEYWORD : TOKEN_NAME);
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
			CASE1('{', TOKEN_LBRACE);
			CASE1('}', TOKEN_RBRACE);
			CASE1('[', TOKEN_LBRACKET);
			CASE1(']', TOKEN_RBRACKET);
			CASE1(',', TOKEN_COMMA);
			CASE1('@', TOKEN_AT);
			CASE1('#', TOKEN_POUND);
			CASE1('?', TOKEN_QUESTION);
			CASE1(';', TOKEN_SEMICOLON);
			CASE1('~', TOKEN_NEG);
			CASE1('!', TOKEN_NOT);
			CASE1(':', TOKEN_COLON);
			CASE1('=', TOKEN_ASSIGN);
			CASE1('^', TOKEN_XOR);
			CASE1('*', TOKEN_MUL);
			CASE1('/', TOKEN_DIV);
			CASE1('%', TOKEN_MOD);
			CASE1('+', TOKEN_ADD);
			CASE1('-', TOKEN_SUB);
			CASE1('&', TOKEN_AND);
			CASE2('>', TOKEN_GT, '=', TOKEN_GTEQ);
			CASE2('<', TOKEN_LT, '=', TOKEN_LTEQ);
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
	stream = buf;
	TODO(line_start = stream);
	TODO(token.pos.name = name ? name : "<string>");
	TODO(token.pos.line = 1);
	next_token();
}

unsigned is_token(TokenKind kind)
{
	return token.kind == kind;
}

unsigned is_token_eof(void)
{
	return token.kind == TOKEN_EOF;
}

unsigned is_token_name(const char *name)
{
	return token.kind == TOKEN_NAME && token.name == name;
}

unsigned is_keyword(const char *name)
{
	return is_token(TOKEN_KEYWORD) && token.name == name;
}

unsigned match_keyword(const char *name)
{
	if (is_keyword(name)) {
		next_token();
		return 1;
	} else {
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
		fatal_error_here("Expected token %s, got %s",
				token_kind_name(kind), token_info());
		return 0;
	}
}


#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) \
		&& match_token(TOKEN_NAME))
#define assert_token_int(x) assert(token.int_val == (x) \
		&& match_token(TOKEN_INT))
#define assert_token_float(x) assert(token.float_val == (x) \
		&& match_token(TOKEN_FLOAT))
#define assert_token_str(x) assert(strcmp(token.str_val, (x)) == 0 \
		&& match_token(TOKEN_STR))
#define assert_token_eof() assert(is_token(0))

void test_lex(void)
{
	TODO(keyword_test());

	// Integer literal tests
	init_stream(0, "0 2147483647 042 1111");
	assert_token_int(0);
	assert_token_int(2147483647);
	assert_token_int(42);
	assert_token_int(1111);
	assert_token_eof();

	// Float literal tests
	init_stream(0, "3.14 .123 42.");
	assert_token_float(3.14);
	assert_token_float(.123);
	assert_token_float(42.);
	assert_token_eof();

	// String literal tests
	init_stream(0, "\"foo\" \"a\\nb\"");
	assert_token_str("foo");
	assert_token_str("a\\nb");
	assert_token_eof();

	// Operator tests
	init_stream(0, ": + < <= > >= - *");
	assert_token(TOKEN_COLON);
	assert_token(TOKEN_ADD);
	assert_token(TOKEN_LT);
	assert_token(TOKEN_LTEQ);
	assert_token(TOKEN_GT);
	assert_token(TOKEN_GTEQ);
	assert_token(TOKEN_SUB);
	assert_token(TOKEN_MUL);
	assert_token_eof();

	// Misc tests
	init_stream(0, "XY+(XY)_HELLO1,234+994");
	TODO(assert_token_name("XY"));
	TODO(assert_token(TOKEN_ADD));
	TODO(assert_token(TOKEN_LPAREN));
	TODO(assert_token_name("XY"));
	TODO(assert_token(TOKEN_RPAREN));
	TODO(assert_token_name("_HELLO1"));
	TODO(assert_token(TOKEN_COMMA));
	TODO(assert_token_int(234));
	TODO(assert_token(TOKEN_ADD));
	TODO(assert_token_int(994));
	TODO(assert_token_eof());
}

#undef assert_token
#undef assert_token_name
#undef assert_token_int
#undef assert_token_float
#undef assert_token_str
#undef assert_token_eof

int main(void)
{
	test_buf();
	test_arena();
	test_lex();
	test_map();
	test_str_intern();
	return 0;
}
