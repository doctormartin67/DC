#ifndef EXCEL_H_
#define EXCEL_H_

#include "common.h"

typedef enum TypeKind {
	TYPE_DOUBLE,
	TYPE_STRING,
} TypeKind;

typedef struct Content {
	TypeKind kind;
	Val val;
} Content;

typedef struct Sheet {
	const char *excel_name;
	const char *name;
	const char *sheetId;
	Map *cells;
} Sheet;

typedef struct Excel {
	const char *name;
	Sheet **sheets;
} Excel;

Excel *open_excel(const char *file_name, const char *sheet_name);
void print_excel(Excel *e);
Content *cell_content(Excel *excel, const char *sheet_name, const char *cell);
Val cell_val(Content *content);
void print_content(Content *content);

#endif
