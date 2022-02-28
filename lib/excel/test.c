#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "excel.h"

int main(void)
{
	const char *file_name = "example.xlsx";
	Excel *file = open_excel(file_name, 0);
	assert(!strcmp(cell_val(cell_content(file, "Sheet1", "E5")).s, "2"));
	assert(!strcmp(cell_val(cell_content(file, "Sheet1", "J10")).s, "0"));
	assert(!strcmp(cell_val(cell_content(file, "Sheet2", "F14")).s, "hello"));
	assert(!strcmp(cell_val(cell_content(file, "Sheet2", "C7")).s, "0"));
	assert(!strcmp(cell_val(cell_content(file, "Sheet3", "E5")).s, "helloI"));
	assert(!strcmp(cell_val(cell_content(file, "Sheet3", "I1")).s, "0"));
	return 0;
}

