#ifndef XL
#define XL

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

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
XLfile *createXL(const char *s);
/* s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
   sheet is the number of the sheet to open. Returns NULL when no
   value in cell.
 */
char *cell(const XLfile *xl, unsigned sheet, const char *s);
/* the excel zip has an xml file with all the string literals
   called sharedStrings.
   in the sheet xml files they are listed as a number and so we
   need to retrieve the strings given this number
 */
char *findss(const XLfile *xl, int index);
char *getconcatss(xmlNodePtr cn);
void setsheetnames(XLfile *xl);
void setnodes(XLfile *xl);
unsigned getrow(const char *cell);
void setcol(char s[], const char *cell);
void nextcol(char *next);
void strshift(char *s);
xmlDocPtr getxmlDoc(const char *docname);
xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath);
void freeXL(XLfile *xl);
void createXLzip(const char *s);

#endif
