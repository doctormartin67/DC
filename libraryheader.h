#include <stdio.h>

char *trim(char *);
void createXLzip(char *s);
char *cell(char *s, char *fname, int sheet);
char *replace(const char *str, const char *orig, const char *rep);
int FILEexists(const char *fname);
int DIRexists(const char *dname);
