#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libraryheader.h"

int main(int argc, char **argv) {

  if (argc < 4) {
    printf("Syntax: readCalc \"Excel file name\" sheetname cell\n");
    exit(0);
  }
  XLfile xl;
  setXLvals(&xl, argv[1]);
  printf("%s = %s\n", *(argv+3), cell(*(argv+3), &xl, *(argv+2)));
  printf("%s = %s\n", *(argv+3), cell(*(argv+3), &xl, *(argv+2)));
  
  return 0;
}


