#ifndef XL
#define XL

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "errorexit.h"

#define MAXSHEETS 256

#define NSPREFIX "main"
#define NSURI "http://schemas.openxmlformats.org/spreadsheetml/2006/main"
#define XPATHSS "//main:si" /* used for sharedStrings.xml file */
#define XPATHDATA "//main:row" /* used for all the sheet[0-9].xml files */

typedef struct {
	char fname[PATH_MAX + 1];
	char dirname[PATH_MAX + 1];
	char **sheetname;
	unsigned int sheetcnt;
	xmlDocPtr workbook;
	xmlDocPtr sharedStrings;
	xmlXPathObjectPtr nodesetss;
	xmlDocPtr *sheets;
	xmlXPathObjectPtr *nodesets;
} XLfile;

// s is the name of the excel file to set the values of
XLfile *createXL(const char *restrict s);
/* s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
   sheet is the number of the sheet to open. Returns NULL when no
   value in cell.
 */
char *cell(const XLfile *restrict xl, unsigned sheet,
		const char *restrict s);
/* the excel zip has an xml file with all the string literals
   called sharedStrings.
   in the sheet xml files they are listed as a number and so we
   need to retrieve the strings given this number
 */
char *findss(const XLfile *restrict xl, int index);
char *getconcatss(xmlNodePtr cn);
void setsheetnames(XLfile *restrict xl);
void setnodes(XLfile *restrict xl);
xmlDocPtr getxmlDoc(const char *restrict docname);
xmlXPathObjectPtr getnodeset(const xmlDocPtr restrict doc,
		const xmlChar *restrict xpath);
void freeXL(XLfile *restrict xl);
void createXLzip(const char *s);

/*
 * inline functions
 */
/* 
 * This function will return the row of a given excel cell,
 * for example 11 in the case of "B11"
 */
inline unsigned getrow(const char cell[static 1])
{
	while (!isdigit(*cell)) cell++;
	return atoi(cell);
}

/* 
 * This function will set s to the column of a given excel cell,
 * for example "B11" will set s to "B"
 */
inline void setcol(char s[static 1], const char cell[static 1])
{
	while (!isdigit(*cell)) *s++ = *cell++;
	*s = '\0';
}

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
inline void nextcol(char next[static 1])
{
	char *s = next;
	if ('Z' == *s && (isdigit(*(s + 1)) || 'Z' == *(s + 1))) {
		strshift(s);
		*s = 'A';
		*(s + 1) = 'A';
		if (!isdigit(*(s + 2)))
			*(s + 2) = 'A';

		return;
	}

	while (!isdigit(*s) && '\0' != *s) {
		if (*s <= 'z' && *s >= 'a') errExit("invalid cell [%s]", next);
		s++;
	}
	s--;
	while ('Z' == *s) *s-- = 'A';
	(*s)++;
}

#endif
