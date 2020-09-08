#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LENGTH BUFSIZ * 1000

typedef struct excel {
  char fname[BUFSIZ/2];
  char dirname[BUFSIZ/2];
  struct stat fbuf;
  struct stat dirbuf;
  
} XLfile;

char *trim(char *);
char *strinside(char *s, char *begin, char *end);
char *replace(const char *str, const char *orig, const char *rep);
int FILEexists(const char *fname);
int DIRexists(const char *dname);
void setXLvals(XLfile *xl, char *s);
void createXLzip(XLfile *xl);
char *cell(char *s, XLfile *xl, char *sheet);
int findsheetID(XLfile *xl, char *s);
char *findss(XLfile *xl, int index);
