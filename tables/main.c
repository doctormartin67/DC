#include <stdio.h>

extern int lx(char *, int);

int main() {
  int age;
  int test;
  char *table;
  table = "LXFK";
  for (age = 0; age < 120; age++) {
    test = lx(table, age);
    printf("lx(%s, %d) = %d\n", table, age, test);
  }
  return 0;
}
