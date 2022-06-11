#include <stdio.h> // printf()
#include <assert.h>
#include <math.h> // pow()
#include "common.h"
#include "actfuncs.h"

#define ABS(x) (x) < 0 ? -(x) : (x)

enum {N = 1024 * 1024};

const char *path = "/home/doctormartin67/Coding/tables/tables/";

static void test_life_tables(void)
{
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

static void test_npx(void)
{
	double nPx = 0.0;
	const char *table = 0;
	char *buf = 0;
	buf_printf(buf, "%s%s", path, "LXMR");
	table = str_intern(buf);
	for (unsigned i = 0; i < N; i++) {
		nPx = npx(table, i%110, (i+1)%110, 0);
		nPx = npx(table, i%110, (i+1)%110, -3);
		nPx = npx(table, i%107, (i+1)%107, 3);
		assert(1 == npx(table, 40, 40, 0));
		assert(0 == npx(table, 0, 130, 0));
		assert(ABS(nPx - (double)lx(table, (i+1)%107 + 3)
				/ lx(table, i%107 + 3)) < EPS);
	}
	buf_free(buf);
}

static void test_nEx(void)
{
	double nex = 0.0;
	double ageX = 0.0;
	double ageXn = 0.0;
	const char *table = 0;
	char *buf = 0;
	buf_printf(buf, "%s%s", path, "LXFK'");
	table = str_intern(buf);
	for (unsigned i = 0; i < N; i++) {
		ageX = i%110;
		ageXn = (i+1)%110;
		nex = nEx(table, 0.0, 0.0, ageX, ageXn, -3);
		assert(ABS(nex - npx(table, ageX, ageXn, -3)) < EPS);
		nex = nEx(table, 0.01, 0.0, ageX, ageXn, -3);
		assert(ABS(nex - npx(table, ageX, ageXn, -3)
					* pow(1 + 0.01, ageX - ageXn)) < EPS);
		nex = nEx(table, 0.012, 0.001, ageX, ageXn, -3);
		assert(ABS(nex - npx(table, ageX, ageXn, -3)
					* pow((1 + 0.012) / (1 + 0.001),
						ageX - ageXn)) < EPS);
	}
	for (unsigned i = 0; i < N; i++) {
		ageX = i%110 + 1.0/12;
		ageXn = i%110 + 20.0/12;
		nex = nEx(table, 0.012, 0.001, ageX, ageXn, -5);
		assert(ABS(nex - npx(table, ageX, ageXn, -5)
					* pow((1 + 0.012) / (1 + 0.001),
						ageX - ageXn)) < EPS);
	}
	buf_free(buf);
}

static void test_axn(void)
{
	double ax = 0.0;
	double nex = 0.0;
	double ageX = 0.0;
	double ageXn = 0.0;
	const char *table = 0;
	char *buf = 0;
	buf_printf(buf, "%s%s", path, "LXMK");
	table = str_intern(buf);
	for (unsigned i = 0; i < N; i++) {
		ax = axn(table, 0.02, 0.001, 1, 12, i, i, -3);
		assert(!ax);
		ax = axn(table, 0.02, 0.001, 0, 1, i, i+1, -3);
		assert(1 == ax);
		ax = axn(table, 0.02, 0.001, 1, 1, i, i+1, -3);
		nex = nEx(table, 0.02, 0.001, i, i+1, -3);
		assert(ABS(nex - ax) < EPS);
	}
	buf_free(buf);
	buf_printf(buf, "%s%s", path, "Lxnihil");
	table = str_intern(buf);
	for (unsigned i = 0; i < N; i++) {
		ageX = i%110;
		ageXn = i%110 + 1;
		ax = axn(table, 0.0, 0.0, 1, 12, ageX, ageXn, 0);
		assert(1 == ax);
		ageX = i%110;
		ageXn = ageX + 1.0/12;
		ax = axn(table, 0.01, 0.001, 1, 12, ageX, ageXn, 0);
		nex = nEx(table, 0.01, 0.001, ageX, ageXn, 0);
		assert(ABS(ax - nex) < EPS);
	}
	buf_free(buf);
}

int main(void)
{
	test_life_tables();
	test_npx();
	test_nEx();
	test_axn();
	return 0;
}
