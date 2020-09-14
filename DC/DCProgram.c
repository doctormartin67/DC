#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "DCProgram.h"
#include "libraryheader.h"

void setCMvals(XLfile *xl, CurrentMember *cm) {
  setkey(xl, cm);
}

/* used to find the row where the keys lie for the data to be used
   for calculations. If the word KEY isn't found in the data then
   1 is returned */
void setkey(XLfile *xl, CurrentMember *cm) {
  FILE *fp;
  char line[LENGTH];
  char lookup[BUFSIZ];
  char sname[BUFSIZ]; // name of the sheet to open (xml file)
  char *begin;
  int sheetnr;
  int value = 1; // value to return
  int i = 0;
  while (*(xl->sheetname + i) != NULL) {
    if(!(sheetnr = findsheetID(xl, *(xl->sheetname + i))))
      exit(0);
    // create the name of the xml file to find the cell value
    snprintf(sname, sizeof sname, "%s%s%d%s", xl->dirname, "/xl/worksheets/sheet", sheetnr, ".xml");

    if ((fp = fopen(sname, "r")) == NULL) {
      perror(sname);
      exit(1);
    }
    while (fgets(line, LENGTH, fp) != NULL) {
      begin = line;
      strcpy(lookup, "<v>");
    
      if ((begin = strcasestr(begin, "KEY")) == NULL)
	continue;
      begin -= 15;
      begin = strinside(begin, "\">", "</f");
      printf("begin = %.*s\n", 100, begin);
      char *temp = begin;
      while (!isdigit(*temp))
	temp++;
      value = atoi(temp);
      free(begin);
      fclose(fp);
      strcpy(cm->datasheet, *(xl->sheetname + i));
      cm->keyrow = value;
      return;
    }
    i++;
  }
  fclose(fp);
  printf("warning: KEY was not found anywhere, ");
  printf("row 1 is therefore assumed for the key row and ");
  printf("\"sheet1\" is assumed as the data sheet name\n");
  strcpy(cm->datasheet, *xl->sheetname);
  cm->keyrow = 1;
}
