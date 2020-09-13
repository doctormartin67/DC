#include <stdio.h>
#include <string.h>
#include "DCProgram.h"
#include "libraryheader.h"

/* used to find the row where the keys lie for the data to be used
   for calculations. If the word KEY isn't found in the data then
   1 is returned */
int findkeyrow(XLfile *xl, char *sheet) {
  FILE *fp;
  char line[LENGTH];
  char sname[BUFSIZ]; // name of the sheet to open (xml file)
  char *begin;
  int sheetnr;

  if(!(sheetnr = findsheetID(xl, sheet)))
    exit(0);
  // create the name of the xml file to find the cell value
  snprintf(sname, sizeof sname, "%s%s%d%s", xl->dirname, "/xl/worksheets/sheet", sheetnr, ".xml");

  if ((fp = fopen(sname, "r")) == NULL) {
    perror(sname);
    exit(1);
  }
  while (fgets(line, LENGTH, fp) != NULL) {
    begin = line;

    if ((begin = strcasestr(begin, "KEY")) == NULL)
      continue;
    begin -= 25;
    printf("begin = %s\n", begin);
    begin = strinside(begin, "r=\"", "\" s");
    char *temp = begin;
    while (!isdigit(temp++))
      ;
    free(begin);
    fclose(fp);
    return atoi(temp);
  }

  fclose(fp);
  printf("KEY was not found anywhere is the given sheet, ");
  printf("row 1 is therefore assumed for the key row.\n");
  return 1;

}
