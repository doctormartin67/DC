#ifndef LIBRARY_HEADER
#define LIBRARY_HEADER

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <gtk/gtk.h>
#include "hashtable.h"

#ifdef _WINDOWS
#define strcasecmp stricmp
#endif

#define MAXSHEETS 256

#define NSPREFIX "main"
#define NSURI "http://schemas.openxmlformats.org/spreadsheetml/2006/main"
#define XPATHSS "//main:si" /* used for sharedStrings.xml file */
#define XPATHDATA "//main:row" /* used for all the sheet[0-9].xml files */

typedef struct excel {
    char fname[PATH_MAX];
    char dirname[PATH_MAX];
    char **sheetname;
    unsigned int sheetcnt;
    xmlDocPtr workbook;
    xmlDocPtr sharedStrings;
    xmlXPathObjectPtr nodesetss;
    xmlDocPtr *sheets;
    xmlXPathObjectPtr *nodesets;
} XLfile;

//---This section is for Date functionality---
typedef struct date {
    unsigned int XLday; // excel starts counting at 31/12/1899
    int year;
    int month;
    int day;
} Date;
enum months {JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC};
int isleapyear(int year);
void setdate(Date *date);
Date *newDate(unsigned int XLday, int year, int month, int day);
Date *minDate(int, ...);
double calcyears(Date *d1, Date *d2, int m);
void printDate(Date *d);

//---Library Functions---
char *trim(char *);
void upper(char *);
int isfloat(const char *);
int isint(const char *);
int isgarbage(int c);
char *strinside(const char *s, const char *begin, const char *end);
char *replace(const char *str, const char *orig, const char *rep);
int DIRexists(const char *dname);
double min(int, ...);
double max(int, ...);
double sum(double a[], int length); // sum all elements of array
void errExit(const char *format, ...);
void errExitEN(int errnum, const char *format, ...);
xmlDocPtr getxmlDoc(const char *docname);
xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath);
void setnodes(XLfile *xl);
void createXLzip(const char *s);
// s is the name of the excel file to set the values of
XLfile *createXL(const char *s);
void freeXL(XLfile *xl);

/* s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
   sheet is the number of the sheet to open. Returns NULL when no
   value in cell.
 */
char *cell(XLfile *xl, unsigned int sheet, const char *s);
unsigned int getrow(const char *cell);
void setcol(char s[], const char *cell);

/* the excel zip has an xml file with all the string literals
   called sharedStrings.
   in the sheet xml files they are listed as a number and so we
   need to retrieve the strings given this number
 */
char *findss(XLfile *xl, int index);
void setsheetnames(XLfile *xl);
FILE *opensheet(XLfile *xl, char *sheet);
void nextcol(char *next);

#endif
