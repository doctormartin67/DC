#include <stdio.h> // printf()
#include <assert.h>
#include "common.h"
#include "act_funcs.h"

const char *path = "/home/doctormartin67/Coding/tables/tables/";

static void test_life_tables(void)
{
	enum {N = 1024 * 1024 * 32};
	const char *table = 0;
	char *buf = 0;
	buf_printf(buf, "%s%s", path, "LXMR");
	table = str_intern(buf);
	for (unsigned i = 0; i < N; i++) {
		assert(1000000 == lx(table, 0));
		assert(965973 == lx(table, 40));
		assert(23 == lx(table, 110));
		assert(0 == lx(table, 1000));
	}
	buf_free(buf);
	buf_printf(buf, "%s%s", path, "LXFR");
	table = str_intern(buf);
	for (unsigned i = 0; i < N; i++) {
		assert(1000000 == lx(table, 0));
		assert(982954 == lx(table, 40));
		assert(99 == lx(table, 110));
		assert(0 == lx(table, 1000));
	}
	buf_free(buf);
}

int main(void)
{
	test_life_tables();
	return 0;
}
