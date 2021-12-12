#define TODO(x)
//#define TODO(x) printf("TODO\n")

/*
 * one of the most useful tricks in C ever invented.
 * push and pop element in an array of arbitrary type
 */

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

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)
#define IS_POW2(x) (((x) != 0) && ((x) & ((x)-1)) == 0)
#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))
#define ALIGN_DOWN_PTR(p, a) ((void *)ALIGN_DOWN((uintptr_t)(p), (a)))
#define ALIGN_UP_PTR(p, a) ((void *)ALIGN_UP((uintptr_t)(p), (a)))

#define ARENA_ALIGNMENT 8
#define ARENA_BLOCK_SIZE (1024 * 1024)

typedef struct BufHdr {
	size_t len;
	size_t cap;
	char buf[0];
} BufHdr;

typedef struct Arena {
	char *ptr;
	char *end;
	char **blocks;
} Arena;

typedef struct Map {
	uint64_t *keys;
	uint64_t *vals;
	size_t len;
	size_t cap;
} Map;

typedef struct Intern {
	size_t len;
	struct Intern *next;
	char str[0];
} Intern;

void *buf__grow(const void *buf, size_t new_len, size_t elem_size);
char *buf__printf(char *buf, const char *fmt, ...);
void arena_grow(Arena *arena, size_t min_size);
void *arena_alloc(Arena *arena, size_t size);
void arena_free(Arena *arena);
const char *str_intern_range(const char *start, const char *end);
const char *str_intern(const char *str);
