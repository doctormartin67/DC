#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "DCProgram.h"
#include "libraryheader.h"

void setDSvals(XLfile *xl, DataSet *ds) {
  ds->xl = xl;
  setkey(ds);
  countMembers(ds);
  createData(ds);
}

/* used to find the row where the keys lie for the data to be used
   for calculations. If the word KEY isn't found in the data then
   1 is returned */
void setkey(DataSet *ds) {
  FILE *fp;
  XLfile *xl;
  char line[BUFSIZ];
  char sname[BUFSIZ]; // name of the sheet to open (xml file)
  char *begin;
  int sheetnr;
  int value = 1; // value to return
  int i,j = 0;
  xl = ds->xl;
  while (*(xl->sheetname + i) != NULL) {
    fp = opensheet(xl, *(xl->sheetname + i));
  
    while (fgets(line, BUFSIZ, fp) != NULL) {
      begin = line;
    
      if ((begin = strcasestr(begin, "KEY")) == NULL)
	continue;
      begin -= 15;
      begin = strinside(begin, "\">", "</f");
      char *temp = begin;
      while (!isdigit(*temp)) {
	ds->keycolumn[j++] = *temp;
	temp++;
      }
      value = atoi(temp);
      free(begin);
      fclose(fp);
      strcpy(ds->datasheet, *(xl->sheetname + i));
      ds->keyrow = value;
      
      if (strlen(ds->keycolumn) > 3) {
	printf("the key column: %s has a length larger than 3 which should not be ",
	       ds->keycolumn);
	printf("possible in excel, exiting program.\n");
	exit(1);
      }
      printf("Found KEY in\nsheet: %s\ncell: %s%d\n",
	     ds->datasheet, ds->keycolumn, ds->keyrow);
      printf("Setting datasheet to: %s\n", ds->datasheet);
      printf("Data starts at cell: %s%d\n", ds->keycolumn, ds->keyrow);
      return;
    }
    i++;
  }
  fclose(fp);
  printf("warning: KEY was not found anywhere, ");
  printf("row 1 is therefore assumed for the key row, ");
  printf("column A is assumed as key column and ");
  printf("\"sheet1\" is assumed as the data sheet name\n");
  strcpy(ds->datasheet, *xl->sheetname);
  ds->keyrow = 1;
  ds->keycolumn[0] = 'A';
}

void countMembers(DataSet *ds) {
  FILE *fp;
  int count = 0;
  char column[3];
  int irow;
  char srow[6];
  char currentCell[10];
  
  fp = opensheet(ds->xl, ds->datasheet);

  strcpy(column, ds->keycolumn);
  irow = ds->keyrow;
  snprintf(srow, sizeof(srow), "%d", irow);
  strcpy(currentCell, column);
  strcat(currentCell, srow);
  while (cell(fp, currentCell, ds->xl) != NULL) {
    snprintf(srow, sizeof(srow), "%d", ++irow);
    strcpy(currentCell, column);
    strcat(currentCell, srow);    
  }
  ds->membercnt = irow - 1 - ds->keyrow;
  printf("Amount of affiliates in data: %d\n", ds->membercnt);
  fclose(fp);
}

void createData(DataSet *ds) {
  FILE *fp;
  char column[4];
  int irow, i = 0;
  char srow[6];
  char keyCell[10];
  char dataCell[10];

  // Allocate memory for data matrix.
  ds->Data = (Hashtable ***)malloc(sizeof(Hashtable[HASHSIZE][ds->membercnt]));
  fp = opensheet(ds->xl, ds->datasheet);

  strcpy(column, ds->keycolumn);
  irow = ds->keyrow;
  snprintf(srow, sizeof(srow), "%d", irow);
  strcpy(keyCell, column);
  strcat(keyCell, srow);
  
  irow++;
  memset(srow, '\0', sizeof(srow));
  snprintf(srow, sizeof(srow), "%d", irow);
  strcpy(dataCell, column);
  strcat(dataCell, srow);

  while (cell(fp, keyCell, ds->xl) != NULL) {
    printf("keyCell = %s\n", keyCell);
    printf("dataCell = %s\n", dataCell);
    printf("%s = %s\n", keyCell, cell(fp, keyCell, ds->xl));
    printf("%s = %s\n", dataCell, cell(fp, dataCell, ds->xl));
    //set(cell(fp, keyCell, ds->xl), cell(fp, dataCell, ds->xl), *(ds->Data));
    nextcol(keyCell);
    nextcol(dataCell);
  }
  Hashtable *test[HASHSIZE];
  set("hello", "world", test);
  printf("%s\n", get("hello", test)->value);

  fclose(fp);
}
