#ifndef EXCEL_H_
#define EXCEL_H_

#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "errorexit.h"
#include "common.h"

enum {
	CELL_LENGTH = 32,
};

typedef enum ContentKind {
	CONTENT_NONE,
	CONTENT_BOOLEAN,
	CONTENT_INT,
	CONTENT_DOUBLE,
	CONTENT_STRING,
	CONTENT_ERROR,
} ContentKind;

typedef struct Content {
	ContentKind kind;
	Val val;
} Content;

/*
 * cells is a Map of Content pointers
 * f.e. ("B11", &(Content){TYPE_STRING, .s = "KEY"})
 */
typedef struct Sheet {
	const char *excel_name;
	char *name;
	unsigned sheetId;
	Map *cells;
} Sheet;

typedef struct Excel {
	const char *name;
	Sheet **sheets;
} Excel;

typedef struct Record {
	Map *data;
	const char **titles;
	size_t num_titles;
} Record;

typedef struct Database {
	Excel *excel;
	const char **titles;
	Record **records;
	size_t num_titles;
	size_t num_records;
} Database;

Excel *open_excel(const char *file_name, const char *sheet_name);
void close_excel(Excel *excel);
Content *cell_content(Excel *excel, const char *sheet_name, const char *cell);
int record_int(const Database *db, size_t num_record, const char *title);
double record_double(const Database *db, size_t num_record, const char *title);
const char *record_string(const Database *db, size_t num_record,
		const char *title);
bool record_boolean(const Database *db, size_t num_record, const char *title);

Val cell_val(Excel *excel, const char *sheet_name, const char *cell);
void print_excel(Excel *e);
Database *open_database(const char *file_name, const char *sheet_name,
		const char *cell);
void close_database(Database *db);
char *nextrow(char next[static 1]);

#endif
