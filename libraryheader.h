#ifndef LIBRARY_HEADER
#define LIBRARY_HEADER

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "hashtable.h"

#define LENGTH BUFSIZ * 1000

typedef struct excel {
  char fname[BUFSIZ/8];
  char dirname[BUFSIZ/8];
  char *sheetname[256];
  struct stat fbuf;
  struct stat dirbuf;
  Hashtable **cells;
} XLfile;

char *trim(char *);
char *strinside(char *s, char *begin, char *end);
char *replace(const char *str, const char *orig, const char *rep);
int FILEexists(const char *fname);
int DIRexists(const char *dname);

// s is the name of the excel file to set the values of
void setXLvals(XLfile *xl, char *s);
void createXLzip(XLfile *xl);

/* s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
   sheet is the number of the sheet to open,
*/
char *cell(char *s, XLfile *xl, char *sheet);

/* the excel zip has an xml file with all the string literals
   called sharedStrings.
   in the sheet xml files they are listed as a number and so we
   need to retrieve the strings given this number
*/
int findsheetID(XLfile *xl, char *s);
char *findss(XLfile *xl, int index);
void setsheetnames(XLfile *xl);

/* awk is used to create a text file with all the cell values printed per line.
   createDMfile will check whether that text file has been created and also when
   it was created and will create it if neccessary and assign sname with the correct
   name of the file. */
void createDMfile(char *DMname, XLfile *xl, char *sheet);

#endif
