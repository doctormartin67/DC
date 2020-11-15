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
  setCMvals(&ds);

  // Here the loop of all affiliates will start
  setassumptions(ds.cm);

  //---BEGIN LOOP---
  // This loops through all the years of an affiliate and makes the calculations
  for (int k; k < MAXPROJ; k++) {
    ;
  }
  // create excel file to print results
  printresults(&ds);
  return 0;
}
