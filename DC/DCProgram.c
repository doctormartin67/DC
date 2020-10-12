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
  char *temp;
  
  fp = opensheet(ds->xl, ds->datasheet);

  strcpy(column, ds->keycolumn);
  irow = ds->keyrow;
  snprintf(srow, sizeof(srow), "%d", irow);
  strcpy(currentCell, column);
  strcat(currentCell, srow);
  while ((temp = cell(fp, currentCell, ds->xl)) != NULL) {
    snprintf(srow, sizeof(srow), "%d", ++irow);
    strcpy(currentCell, column);
    strcat(currentCell, srow);    
  }
  ds->membercnt = irow - 1 - ds->keyrow;
  printf("Amount of affiliates in data: %d\n", ds->membercnt);
  free(temp);
  fclose(fp);
}

/* Excel is just a bunch of cells of the form O11, DC103, ...
   This function will create a pointer to an array of arrays of pointers to 
   hashtables, where we will store all the cells for our data. We will 
   reference cell O11 as an example going further.*/

void createData(DataSet *ds) {
  FILE *fp;
  char column[4]; // This holds the column of the beginning of our data cell, for example O
  int irow; // This holds the row of the beginning of our data cell, for example 11
  char srow[6]; /* This holds the row in string form of the beginning of our data cell, 
		   for example "11" */
  char keyCell[10]; // This will hold the cell of a key for the hashtable, for example O11
  char dataCell[10]; /* This will hold the cell of data corresponding to a key
			for the hashtable, for example O12.*/
  char *key; // This will hold the key for the hashtable, for example SAL (for salary).
  char *data; // This will hold the value of the data, for example 2.391,30.

  // This opens the data sheet (made with awk).
  fp = opensheet(ds->xl, ds->datasheet);

  // Here we set the initial key cell, for example B11
  strcpy(column, ds->keycolumn);
  irow = ds->keyrow;
  snprintf(srow, sizeof(srow), "%d", irow);
  strcpy(keyCell, column);
  strcat(keyCell, srow);

  // Here we set the initial data cell, for example B12
  irow++;
  memset(srow, '\0', sizeof(srow));
  snprintf(srow, sizeof(srow), "%d", irow);
  strcpy(dataCell, column);
  strcat(dataCell, srow);
  
  // Allocate memory for data matrix and initialise to NULL pointer
  ds->Data = (Hashtable ***)malloc(ds->membercnt * sizeof(Hashtable **));
  for (int k = 0; k < ds->membercnt; k++) {
    *(ds->Data + k) = (Hashtable **)malloc(HASHSIZE * sizeof(Hashtable *));
    for (int l = 0; l < HASHSIZE; l++)
      *(*(ds->Data + k) + l) = NULL;
  }

  // start populating Hashtable
  for (int i = 0; i < ds->membercnt; i++) {
  
    // Set the initial key and corresponding data
    key = cell(fp, keyCell, ds->xl);
    data = cell(fp, dataCell, ds->xl);

    while (key != NULL) {
    
      if (data == NULL)
	set(cell(fp, keyCell, ds->xl), "0", *(ds->Data + i));
      else {
	set(cell(fp, keyCell, ds->xl), cell(fp, dataCell, ds->xl), *(ds->Data + i));
      }

      // Here we update cell for loop, for example O11 becomes P11
      nextcol(keyCell);
      nextcol(dataCell);
      key = cell(fp, keyCell, ds->xl);
      data = cell(fp, dataCell, ds->xl);
    }

    printf("NAAM %d = %s\n", i + 1, get("NAAM", *(ds->Data + i))->value);

    // Iterate through all affiliates by updating keyCell
    memset(srow, '\0', sizeof(srow));
    memset(keyCell, '\0', sizeof(keyCell));
    snprintf(srow, sizeof(srow), "%d", ds->keyrow);
    strcpy(keyCell, column);
    strcat(keyCell, srow);
    // Iterate through all affiliates by updating dataCell
    irow++;
    memset(srow, '\0', sizeof(srow));
    memset(dataCell, '\0', sizeof(dataCell));
    snprintf(srow, sizeof(srow), "%d", irow);
    strcpy(dataCell, column);
    strcat(dataCell, srow);
    
  }
  fclose(fp);
}
