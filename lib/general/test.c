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

#include "common.h"

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

int main(void)
{
	test_buf();
	test_arena();
	test_map();
	test_str_intern();
	return 0;
}
