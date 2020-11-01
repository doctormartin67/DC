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
  DataSet ds;

  setXLvals(&xl, argv[1]);
  setDSvals(&xl, &ds);

  CurrentMember cm[ds.membercnt];
  printf("before settting cm\n");
  setCMvals(&ds, cm);

  // create excel file to print results
  printresults(&ds);
  return 0;
}
