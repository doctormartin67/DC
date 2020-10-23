#include <stdio.h>
#include <stdlib.h>
#include "libraryheader.h"
#include "hashtable.h"
#include "DCProgram.h"
#include "xlsxwriter.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Syntax: main \"Excel file\"\n");
    exit(0);
  }
    
  XLfile xl;
  DataSet ds;
  
  setXLvals(&xl, argv[1]);
  setDSvals(&xl, &ds);
  lxw_workbook  *workbook  = workbook_new("myexcel.xlsx");
  lxw_worksheet *worksheet = workbook_add_worksheet(workbook, NULL);
  int row = 0;
  int col = 0;
 
  worksheet_write_string(worksheet, row, col, "Hello me!", NULL);
 
  return workbook_close(workbook);
  //return 0;
}
