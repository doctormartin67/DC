#ifndef EXCEL_H_
#define EXCEL_H_

#include <ctype.h>
#include "errorexit.h"
#include "common.h"

typedef enum TypeKind {
	TYPE_DOUBLE,
	TYPE_INT,
	TYPE_STRING,
	TYPE_ERROR,
} TypeKind;

typedef struct Content {
	TypeKind kind;
	Val val;
} Content;

/*
 * cells is a Map of Content pointers
 * f.e. ("B11", &(Content){TYPE_STRING, .s = "KEY"})
 */
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

typedef struct Database {
	Excel *excel;
	const char **titles;
	Map **records;
	size_t num_titles;
	size_t num_records;
} Database;

Excel *open_excel(const char *file_name, const char *sheet_name);
void close_excel(Excel *excel);
Content *cell_content(Excel *excel, const char *sheet_name, const char *cell);
Val cell_val(Excel *excel, const char *sheet_name, const char *cell);
void print_excel(Excel *e);
void print_content(Content *content);
Database *open_database(const char *file_name, const char *sheet_name,
		const char *cell);
void close_database(Database *db);

/*
 * helper function for nextcol. it shifts all the char's one place to the
 * right. It is needed in the case where the column is 'Z' or 'ZZ', meaning
 * the next column would be 'AA' or 'AAA' respectively. As an example,
 * Z11 becomes AA11
 */
inline void strshift(char s[static 1])
{
	char *t = s;

	while (*s) s++;
	*(s + 1) = '\0';

	while (s > t) {
		*s = *(s - 1);
		s--;
	}
}

/*
 * sets 'next' to the next cell to the right, f.e. B11 -> C11
 */
inline char *nextcol(char next[static 1])
{
	char *s = next;
	if ('Z' == *s && (isdigit(*(s + 1)) || 'Z' == *(s + 1))) {
		strshift(s);
		*s = 'A';
		*(s + 1) = 'A';
		if (!isdigit(*(s + 2)))
			*(s + 2) = 'A';

		return next;
	}

	while (!isdigit(*s) && '\0' != *s) {
		if (*s <= 'z' && *s >= 'a') die("invalid cell [%s]", next);
		s++;
	}
	s--;
	while ('Z' == *s) *s-- = 'A';
	(*s)++;
	return next;
}

#endif
