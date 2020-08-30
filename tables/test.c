#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 15

extern int lx(char *, int);

int main(int argc, char **argv) {
  int age;
  int test;
  char *tables;
  FILE *lt;
  char line[MAXLINE];
  
  tables = "tables.txt";

  if ((lt = fopen(tables, "r")) == NULL) {
    fprintf(stderr, "can't open %s\n", tables);
    exit(1);
  }

  age = atoi(*(argv+1));
  
  while (fgets(line, MAXLINE, lt)){
    line[strcspn(line, "\n")] = '\0'; //remove newline character
    test = lx(line, age);
    printf("lx(%s, %d) = %d\n", line, age, test);
    test = lx(line, age);
    printf("lx(%s, %d) = %d\n", line, age, test);
  }
  return 0;
}
