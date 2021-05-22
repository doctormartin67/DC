#ifndef LIBRARY_HEADER
#define LIBRARY_HEADER

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "hashtable.h"

enum {MAXLENGTH = 256};

typedef struct excel {
    char fname[MAXLENGTH];
    char dirname[MAXLENGTH];
    char *sheetname[MAXLENGTH];
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
char *strinside(const char *s, const char *begin, const char *end);
char *replace(const char *str, const char *orig, const char *rep);
int FILEexists(const char *fname);
int DIRexists(const char *dname);
double min(int, ...);
double max(int, ...);
double sum(double a[], int length); // sum all elements of array

// s is the name of the excel file to set the values of
void setXLvals(XLfile *xl, const char *s);

/* s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
   sheet is the number of the sheet to open. Returns NULL when no
   value in cell.
 */
char *cell(FILE *fp, const char *s, XLfile *xl);

/* the excel zip has an xml file with all the string literals
   called sharedStrings.
   in the sheet xml files they are listed as a number and so we
   need to retrieve the strings given this number
 */
int findsheetID(XLfile *xl, char *s);
char *findss(XLfile *xl, int index);
void setsheetnames(XLfile *xl);
FILE *opensheet(XLfile *xl, char *sheet);
void nextcol(char *next);
char *valueincell(XLfile *xl, char *line, const char *find);

#endif
