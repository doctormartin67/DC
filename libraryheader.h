#ifndef LIBRARY_HEADER
#define LIBRARY_HEADER

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "hashtable.h"

typedef struct excel {
  char fname[BUFSIZ/8];
  char dirname[BUFSIZ/8];
  char *sheetname[256];
} XLfile;

char *trim(char *);
char *strinside(char *s, char *begin, char *end);
char *replace(const char *str, const char *orig, const char *rep);
int FILEexists(const char *fname);
int DIRexists(const char *dname);

// s is the name of the excel file to set the values of
void setXLvals(XLfile *xl, char *s);

/* s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
   sheet is the number of the sheet to open. Returns NULL when no
   value in cell.
*/
char *cell(FILE *fp, char *s, XLfile *xl);

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
char *valueincell(XLfile *xl, char *line, char *find);

#endif
