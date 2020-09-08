#include <stdio.h>
#include <stdlib.h>
#include "libraryheader.h"

int main(int argc, char **argv) {

  char test[] = "gerjgr<v>hello</v>dfijdsjf";
  char *pt;
  pt = strinside(test, "<v>", "</v>");
  printf("test = %s\n", test);
  printf("pt = %s\n", pt);
  free(pt);
  
  return 0;
}
