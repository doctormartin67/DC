#include <stdio.h>
#include <stdlib.h>
#include "libraryheader.h"
#include "hashtable.h"
#include "DCProgram.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Syntax: main \"Excel file\"\n");
    exit(0);
  }
    
  XLfile xl;
  CurrentMember cm;
  
  setXLvals(&xl, argv[1]);
  setkey(&xl, &cm);
  printf("keyrow = %d and datasheet = %s\n", cm.keyrow, cm.datasheet);
  return 0;
}
