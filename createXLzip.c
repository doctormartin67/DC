#include <stdio.h>
#include <stdlib.h>
#include "libraryheader.h"

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("Syntax: createXLzip \"excel file\"\n");
    exit(0);
  }
    
  createXLzip(argv[1]);
  
  return 0;
}
