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
   // unzip recently modified excel file
  if ((xl->fbuf).st_mtime > (xl->dirbuf).st_mtime) {
    createXLzip(xl);
  }
  setsheetnames(xl);
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
char *cell(char *s, XLfile *xl, char *sheet) {
  
  FILE *fp;
  char line[LENGTH];
  char lookup[BUFSIZ]; /* string to lookup in xml file to find cell value.
			  is of the form r="A3 for cell A3*/
  char sname[BUFSIZ]; // name of the sheet to open (xml file)
  char *begin;
  char *end;
  int sheetnr;
  static char value[BUFSIZ]; // value of cell to return (string)

  if(!(sheetnr = findsheetID(xl, sheet)))
    exit(0);
  // create the name of the xml file to find the cell value
  snprintf(sname, sizeof sname, "%s%s%d%s", xl->dirname, "/xl/worksheets/sheet", sheetnr, ".xml");

  if ((fp = fopen(sname, "r")) == NULL) {
    perror(sname);
    exit(1);
  }
  while (fgets(line, LENGTH, fp) != NULL) {
    strcpy(lookup, "r=\"");
    strcat(lookup, s);
    strcat(lookup, "\"");
    begin = line;

    if ((begin = strstr(begin, lookup)) == NULL)
      continue;
    begin = strinside(begin, "<v>", "</v>");
    strcpy(value, begin);
    free(begin);
    fclose(fp);
    return value;
  }

  fclose(fp);
  printf("No value in cell %s, returning 0\n", s);
  return NULL;
}

int findsheetID(XLfile *xl, char *s) {

  int sheet = 0;
  while (*(xl->sheetname + sheet) != NULL) {
    if (strcmp(*(xl->sheetname + sheet), s) == 0)
      return sheet + 1;
    sheet++;
  }
  
  printf("Sheet name \"%s\" not found.\n", s);
  printf("Make sure you spelt it correctly, it is case sensitive.\n");
  printf("The following sheets were found:\n");
  printf("---------------------\n");
  sheet = 0;
  while (*(xl->sheetname + sheet) != NULL) {
    printf(">%s<\n", *(xl->sheetname + sheet));
    sheet++;
  }
  printf("---------------------\n");
  printf("Please select one of the above sheets (excluding > and <).\n");
  printf("Returning 0...\n");
  return 0;
}

/* the excel zip has an xml file with all the string literals
   called sharedStrings.
   in the sheet xml files they are listed as a number and so we
   need to retrieve the strings given this number
*/
char *findss(XLfile *xl, int index) {
  FILE *fp;
  char line[LENGTH];
  char sname[BUFSIZ];
  char *begin;
  int count = 0;
  int dcount = 0; /* in the file sharedStrings there are "preserve" tags
		     that are counted double because they are actually
		     in one cell but are there twice. you can recognise
		     this as:
		     <rPr>
		     <sz val="10"/>
		     <color rgb="FF000000"/>
		     <rFont val="Calibri"/>
		     <family val="2"/>
		     <charset val="1"/>
		     </rPr>
		  */
  char *pdcount;
  static char value[BUFSIZ]; // value of cell to return (string)
   // create the name of the xml file to find the sheetID
  snprintf(sname, sizeof sname, "%s%s", xl->dirname, "/xl/sharedStrings.xml");
  if ((fp = fopen(sname, "r")) == NULL) {
    perror(sname);
    exit(1);
  }
  while (fgets(line, LENGTH, fp) != NULL) {
    // check if preserve is in current line
    if ((begin = strstr(line, "=\"preserve\"")) == NULL)
      continue;
    // skip through all the preserves
    while (count != index) {
      begin++;
      pdcount = begin;
      if ((begin = strstr(begin, "=\"preserve\"")) == NULL)
	break;
      count++;
      // temporarily set null character to see if we have a double count
      *begin = '\0';
      if (strstr(pdcount, "<rPr>") != NULL)
	dcount++;
      *begin = '=';
      if (dcount > 0 && dcount % 2 == 0) {
	count--;
	dcount = 0;
      }
    }
    if(count == index) {
      begin = strinside(begin, "erve\">", "</t>");
      strcpy(value, begin);
      free(begin);
      fclose(fp);
      return value;
    }
  }

  fclose(fp);
  printf("Couldn't find anything in sharedStrings.xml. Returning NULL...\n");
  return NULL;
}

void setsheetnames(XLfile *xl) {
  FILE *fp;
  char line[BUFSIZ];
  char sname[BUFSIZ];
  char *begin;
  int sheet = 0;

  // create the name of the xml file to find the sheet names
  snprintf(sname, sizeof sname, "%s%s", xl->dirname, "/xl/workbook.xml");
  if ((fp = fopen(sname, "r")) == NULL) {
    perror(sname);
    exit(1);
  }
  while (fgets(line, LENGTH, fp) != NULL) {
    begin = line;
    while ((begin = strstr(begin, "<sheet name=")) != NULL) {
      *(xl->sheetname + sheet) = strinside(begin, "<sheet name=\"", "\" sheetId");
      sheet++;
      begin += strlen("<sheet name=");
    }
  }
  // final pointer is just a null pointer
  *(xl->sheetname + sheet) = NULL;
  fclose(fp);
}
