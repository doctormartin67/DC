#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 15

extern int lx(char *, int);

int main() {
  int age;
  int test;
  age = 40;
  test = lx("Lxgbm76", age);
  printf("lx(%s, %d) = %d\n", "Lxgbm76", age, test);
  return 0;
}
