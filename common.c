#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "common.h"

void fatal(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("FATAL: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
	exit(1);
}

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

uint64_t hash_uint64(uint64_t x)
{
	x *= 0xff51afd7ed558ccd;
	x ^= x >> 32;
	return x;
}

uint64_t hash_ptr(const void *ptr)
{
	return hash_uint64((uintptr_t)ptr);
}

uint64_t hash_mix(uint64_t x, uint64_t y)
{
	x ^= y;
	x *= 0xff51afd7ed558ccd;
	x ^= x >> 32;
	return x;
}

uint64_t hash_bytes(const void *ptr, size_t len)
{
	uint64_t x = 0xcbf29ce484222325;
	const char *buf = (const char *)ptr;
	for (size_t i = 0; i < len; i++) {
		x ^= buf[i];
		x *= 0x100000001b3;
		x ^= x >> 32;
	}
	return x;
}

static uint64_t map_get_uint64_from_uint64(Map *map, uint64_t key)
{
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

static void map_put_uint64_from_uint64(Map *map, uint64_t key, uint64_t val);

static void map_grow(Map *map, size_t new_cap)
{
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

static void map_put_uint64_from_uint64(Map *map, uint64_t key, uint64_t val)
{
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

static void *map_get(Map *map, const void *key)
{
	return (void *)(uintptr_t)map_get_uint64_from_uint64(map, (uint64_t)(uintptr_t)key);
}

static void map_put(Map *map, const void *key, void *val)
{
	map_put_uint64_from_uint64(map, (uint64_t)(uintptr_t)key, (uint64_t)(uintptr_t)val);
}

void *map_get_from_uint64(Map *map, uint64_t key)
{
	return (void *)(uintptr_t)map_get_uint64_from_uint64(map, key);
}

void map_put_from_uint64(Map *map, uint64_t key, void *val)
{
	map_put_uint64_from_uint64(map, key, (uint64_t)(uintptr_t)val);
}

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
