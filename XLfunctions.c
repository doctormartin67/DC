#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "libraryheader.h"

// s is the name of the excel file to set the values of
void setXLvals(XLfile *xl, char *s) {
  char *temp;
  strcpy(xl->fname, s);
  if ((temp = strstr(s, ".xls")) == NULL || !FILEexists(s)) {// not an excel file
    printf("Please select an valid excel file.\n");
    printf("Exiting program.\n");
    exit(0);
  }
  *temp = '\0';
  strcpy(xl->dirname, s);

  // enter the stats into both buffers. for DIR we check if a zip has been created.
  if (stat(xl->fname, &xl->fbuf) < 0) {
    perror(xl->fname);
    exit(1);
  }
  if (!DIRexists(xl->dirname))
    createXLzip(xl);
  else
  if (stat(xl->dirname, &xl->dirbuf) < 0) {
    perror(xl->dirname);
    exit(1);
  }
}

// create a zip file of an excel document and extract
void createXLzip(XLfile *xl) {
  char *s;
  char *t;
  char *end;
  char command[BUFSIZ];
  // we need to replace spaces with "\ " for the command
  t = replace(xl->dirname, " ", "\\ ");
  s = replace(xl->fname, " ", "\\ ");
  
  snprintf(command, sizeof command, "%s%s%s%s%s", "cp ", s, " ", t, ".zip");
  system(command);

  // *******unzip zip file*******
  
  // make directory for all files
  if (!DIRexists(xl->dirname)) {
    printf("command = %s\n", command);
    snprintf(command, sizeof command, "%s%s", "mkdir ", t);
    system(command);
  }
  else {
    snprintf(command, sizeof command, "%s%s", "rm -rf ", t);
    system(command);
  }
  // unzip in directory
  snprintf(command, sizeof command, "%s%s%s%s", "unzip -q ", t, ".zip -d ", t);
  if(system(command)) {
    printf(">>>>>>");
    printf("THE EXCEL FILE YOU ARE TRYING TO OPEN MIGHT HAVE A PASSWORD.\n");
    printf(">>>>>>");
    printf("REMOVE THE PASSWORD FROM THE FILE AND TRY AGAIN.\n");
    exit(1);
  }
  free(t);
  free(s);
}

/* s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
   sheet is the number of the sheet to open,
*/
char *cell(char *s, XLfile *xl, int sheet) {
  
  FILE *fp;
  char line[LENGTH];
  char lookup[BUFSIZ]; /* string to lookup in xml file to find cell value.
			  is of the form r="A3 for cell A3*/
  char sname[BUFSIZ]; // name of the sheet to open (xml file)
  char *begin;
  char *end;
  char *temp;
  static char value[BUFSIZ]; // value of cell to return (string)
  
  // unzip recently modified excel file
  if ((xl->fbuf).st_mtime > (xl->dirbuf).st_mtime) {
    createXLzip(xl);
  }

  // create the name of the xml file to find the cell value
  snprintf(sname, sizeof sname, "%s%s%d%s", xl->dirname, "/xl/worksheets/sheet", sheet, ".xml");

  if ((fp = fopen(sname, "r")) == NULL) {
    perror(sname);
    exit(1);
  }
  while (fgets(line, LENGTH, fp) != NULL) {
    strcpy(lookup, "r=\"");
    strcat(lookup, s);
    strcat(lookup, "\"");
    begin = line;
    temp = begin;
    int count = 0;
    while ((temp = strstr(temp, "r=\"1\"")) != NULL) {
      count++;
      begin = temp;
      temp++;
    }
    printf("%s\n", line);
    if ((begin = strstr(begin, lookup)) == NULL)
      continue;
    begin = strstr(begin, "<v>");
    begin += 3;
    end = strstr(begin, "</v>");
    printf("begin = %.*s\n", 100, begin);
    *end = '\0';
    strcpy(value, begin);
    *end = '<';
    printf("count = %d\n", count);
    return value;
  }

  fclose(fp);
  printf("No value in cell %s, returning 0\n", s);
  return NULL;
}

