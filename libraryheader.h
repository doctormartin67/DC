#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LENGTH BUFSIZ * 1000

typedef struct excel {
  char fname[256];
  char dirname[256];
  struct stat fbuf;
  struct stat dirbuf;
} XLfile;

char *trim(char *);
void setXLvals(XLfile *xl, char *s);
void createXLzip(XLfile *xl);
char *cell(char *s, XLfile *xl, int sheet);
char *replace(const char *str, const char *orig, const char *rep);
int FILEexists(const char *fname);
int DIRexists(const char *dname);
