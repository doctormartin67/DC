#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "excel.h"

#define assert_null(sheet, cell) \
	assert(!cell_content(file, sheet, cell));

#define assert_int(sheet, cell, n) \
	assert(cell_content(file, sheet, cell)); \
	assert(TYPE_INT == cell_content(file, sheet, cell)->kind); \
	assert(n == cell_content(file, sheet, cell)->val.i);

#define assert_double(sheet, cell, n) \
	assert(cell_content(file, sheet, cell)); \
	assert(TYPE_DOUBLE == cell_content(file, sheet, cell)->kind); \
	assert(n == cell_content(file, sheet, cell)->val.d);

#define assert_string(sheet, cell, str) \
	assert(cell_content(file, sheet, cell)); \
	assert(TYPE_STRING == cell_content(file, sheet, cell)->kind); \
	assert(!strcmp(str, cell_content(file, sheet, cell)->val.s));

#define assert_error(sheet, cell) \
	assert(cell_content(file, sheet, cell)); \
	assert(TYPE_ERROR == cell_content(file, sheet, cell)->kind); \
	assert(!strcmp("", cell_content(file, sheet, cell)->val.s));

void test_excel(void)
{
	const char *file_name = "example.xlsx";
	Excel *file = open_excel(file_name, 0);
	assert_int("Sheet1", "E5", 2);
	assert_null("Sheet1", "J10");
	assert_double("Sheet1", "D6", 6E-006);
	assert_string("Sheet2", "F14", "hello");
	assert_null("Sheet2", "C7");
	assert_string("Sheet3", "E5", "helloI");
	assert_null("Sheet3", "I1");
	assert_error("Sheet1", "J3");
	close_excel(file);
}

#undef assert_null
#undef assert_int
#undef assert_double
#undef assert_string
#undef assert_error

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


void test_database(void)
{
	const char *file_name = "example.xlsx";
	const char *sheet_name = "Sheet1";
	Database *db = open_database(file_name, sheet_name, "D3");
	assert(db);
	//print_database(db);

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
}

extern char *nextrow(char *);
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

int main(void)
{
	test_excel();
	test_database();
	test_nextrow();
	return 0;
}

