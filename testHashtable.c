#include <stdio.h>
#include <stdlib.h>
#include "libraryheader.h"

int main(int argc, char **argv) {

  XLfile xl;
  xl.cells = (Hashtable **) malloc(sizeof(Hashtable *[HASHSIZE]));
  set("B11", "test1", xl.cells);
  set("B12", "test2", xl.cells);
  printf("B11 = %s\n", get("B11", xl.cells)->value);
  printf("B12 = %s\n", get("B12", xl.cells)->value);
  return 0;
}
