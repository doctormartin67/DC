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
    snprintf(sname, sizeof(sname), "%s%s%s%s", xl->dirname, "/", *(xl->sheetname + i), ".txt");
    if ((fp = fopen(sname, "r")) == NULL) {
      fprintf(stderr, "Error in function setkey:\n");
      perror(sname);
      exit(1);
    }
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
  int count = 0;
  char column[3];
  int irow;
  char srow[6];
  char currentCell[10];

  if (strlen(ds->keycolumn) > 3) {
    printf("the key column: %s has a length larger than 3 which should not be ", ds->keycolumn);
    printf("possible in excel, exiting program.\n");
    exit(1);
  }
  strcpy(column, ds->keycolumn);
  irow = ds->keyrow;
  snprintf(srow, sizeof(srow), "%d", irow);
  strcpy(currentCell, column);
  strcat(currentCell, srow);
  while (cell(currentCell, ds->xl, ds->datasheet) != NULL) {
    snprintf(srow, sizeof(srow), "%d", ++irow);
    strcpy(currentCell, column);
    strcat(currentCell, srow);    
  }
  ds->membercnt = irow - 1 - ds->keyrow;
}
