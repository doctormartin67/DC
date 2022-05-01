#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "excel.h"

#define assert_null(sheet, cell) \
	content = cell_content(file, sheet, cell); \
	assert(!content);

#define assert_boolean(sheet, cell, n) \
	content = cell_content(file, sheet, cell); \
	assert(content); \
	assert(CONTENT_BOOLEAN == content->kind); \
	assert(n == content->val.b);

#define assert_int(sheet, cell, n) \
	content = cell_content(file, sheet, cell); \
	assert(content); \
	assert(CONTENT_INT == content->kind); \
	assert(n == content->val.i);

#define assert_double(sheet, cell, n) \
	content = cell_content(file, sheet, cell); \
	assert(content); \
	assert(CONTENT_DOUBLE == content->kind); \
	assert(n == content->val.d);

#define assert_string(sheet, cell, str) \
	content = cell_content(file, sheet, cell); \
	assert(content); \
	assert(CONTENT_STRING == content->kind); \
	assert(!strcmp(str, content->val.s));

#define assert_error(sheet, cell) \
	content = cell_content(file, sheet, cell); \
	assert(content); \
	assert(CONTENT_ERROR == content->kind); \
	assert(!strcmp("", content->val.s));

void test_excel(const char *file_name)
{
	if (!file_name) {
		file_name = "example.xlsx";
		Excel *file = open_excel(file_name, 0);
		Content *content = 0;
		assert_int("Sheet1", "E5", 2);
		assert_null("Sheet1", "J10");
		assert_double("Sheet1", "D6", 6E-006);
		assert_string("Sheet2", "F14", "hello");
		assert_null("Sheet2", "C7");
		assert_string("Sheet3", "E5", "helloI");
		assert_null("Sheet3", "I1");
		assert_error("Sheet1", "J3");
		close_excel(file);
	} else {
		Excel *file = open_excel(file_name, 0);
		Content *content = 0;
		assert_int("Data", "S18", 65);
		assert_null("Data", "AE7");
		assert_double("Data", "AG329", 0.0475);
		assert_string("Data", "F327", "ACT");
		assert_null("Data", "L331");
		assert_string("Data", "F323", "DEF");
		assert_error("Data", "LL8");
		close_excel(file);
	}
}

#undef assert_null
#undef assert_boolean
#undef assert_int
#undef assert_double
#undef assert_string
#undef assert_error

#define assert_boolean(n, title, val) \
	b = record_boolean(db, n, title); \
	assert(val == b);

#define assert_int(n, title, val) \
	i = record_int(db, n, title); \
	assert(val == i);

#define assert_double(n, title, val) \
	d = record_double(db, n, title); \
	assert(val == d);

#define assert_string(n, title, s) \
	str = record_string(db, n, title); \
	assert(str); \
	assert(!strcmp(str, s));


void test_database(const char *file_name)
{
	if (!file_name) {
		file_name = "example.xlsx";
		const char *sheet_name = "Sheet1";
		Database *db = open_database(file_name, sheet_name, "D3");
		assert(db);

		int i = 0;
		double d = 0.0;
		const char *str = 0;
		assert_int(0, "Title 2", 1);
		assert_int(1, "Title 1", 2);
		assert_double(3, "Title 1", 2.2);
		assert_double(2, "Title 1", 6E-06);
		assert_string(2, "Title 3", "gwl");	
		assert_string(4, "Title 4", "Muriel");	

		assert_string(4, "Title 1", "");	
		assert_int(4, "Title 1", 0);	
		assert_double(4, "Title 1", 0.0);	

		assert_string(4, "Title 3", "");	
		assert_int(4, "Title 3", 0);	
		assert_double(4, "Title 3", 0.0);	

		close_database(db);

		sheet_name = "Sheet2";
		db = open_database(file_name, sheet_name, "D12");
		close_database(db);
	} else {
		const char *sheet_name = "Data";
		Database *db = open_database(file_name, sheet_name, "B11");
		assert(db);

		bool b = false;
		int i = 0;
		double d = 0.0;
		const char *str = 0;
		assert_int(0, "NO REGLEMENT", 14286);
		assert_int(4, "SEX", 2);
		assert_double(317, "SAL", 2913.91);
		assert_double(5, "ART24_C_GEN1", 354.56);
		assert_string(2, "TARIEF", "UKMS");	
		assert_string(4, "STATUS", "ACT");	

		assert_string(5, "STATUS", "DEF");	
		assert_int(20, "NRA", 65);	
		assert_double(29, "RES_A_GEN1", 125.8);	

		assert_string(100, "MS", "M");
		assert_int(4, "# ENF", 0);	
		assert_double(10, "TAUX_A_GEN4", 0.01);	

		assert_boolean(4, "ACTIVE CONTRACT", true);	

		close_database(db);
	}
}

void test_nextrow(void)
{
	char row[32];
	char *next = 0;

	snprintf(row, sizeof(row), "%s", "B11");
	next = nextrow(row);
	assert(!strcmp(next, "B12"));

	snprintf(row, sizeof(row), "%s", "B99");
	next = nextrow(row);
	assert(!strcmp(next, "B100"));

	snprintf(row, sizeof(row), "%s", "B990");
	next = nextrow(row);
	assert(!strcmp(next, "B991"));

	snprintf(row, sizeof(row), "%s", "B959");
	next = nextrow(row);
	assert(!strcmp(next, "B960"));

	snprintf(row, sizeof(row), "%s", "B599");
	next = nextrow(row);
	assert(!strcmp(next, "B600"));
}

int main(int argc, char *argv[])
{
	const char *file_name = 0;
	if (2 == argc) {
		file_name = argv[1];
	} else {
		assert(1 == argc);
	}
	enum {N = 64};
	for (unsigned i = 0; i < N; i++) {
		test_excel(file_name);
		test_database(file_name);
		test_nextrow();
	}
	return 0;
}

