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
	char *name;
	char *sheetId;
	Map *cells;
} Sheet;

typedef struct Excel {
	const char *name;
	Sheet **sheets;
} Excel;

Excel *open_excel(const char *file_name, const char *sheet_name);
void close_excel(Excel *excel);
Val cell_val(Excel *excel, const char *sheet_name, const char *cell);
void print_excel(Excel *e);
void print_content(Content *content);

#endif
