#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "excel.h"
#include <stdlib.h>

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

void test_database(void)
{
	const char *file_name = "example.xlsx";
	const char *sheet_name = "Sheet1";
	Database *db = open_database(file_name, sheet_name, "D3");
	assert(db);
	printf("titles:\n");
	for (size_t i = 0; i < db->num_titles; i++) {
		printf("%s\n", db->titles[i]);
	}
	close_database(db);
}

int main(void)
{
	test_excel();
	test_database();
	return 0;
}

