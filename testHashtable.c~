#include <stdio.h>
#include <stdlib.h>
#include "libraryheader.h"

int main(int argc, char **argv) {

  if (argc < 3) {
    printf("Syntax: ./a.out \"excel file name\" index\n");
    exit(0);
  }
  XLfile xl;
  char *test;
  setXLvals(&xl, *(argv+1)); 
  test = findss(&xl, atoi(*(argv+2)));
  printf("test = %s\n", test);
  return 0;
}
