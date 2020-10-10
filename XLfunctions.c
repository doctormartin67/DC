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
  setsheetnames(xl);
}

/* fp is an open sheet, usually the sheet containing the data to evaluate 
   s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
*/
char *cell(FILE *fp, char *s, XLfile *xl) {
  
  char line[BUFSIZ];
  char sname[BUFSIZ/4];
  char *begin;
  char *ss; // string to determine whether I need to call findss or not
  static char value[BUFSIZ]; // value of cell to return (string)

  memset(sname, '0', sizeof(sname));
  strcpy(sname, s);
  strcat(sname, "\"");
  
  while (fgets(line, BUFSIZ, fp) != NULL) {
    begin = line;
    if ((begin = strstr(begin, sname)) == NULL)
      continue;
    if ((ss = strinside(begin, "t=\"", "\">")) == NULL) {     
      ss = "n";
      printf("warning: Couldn't determine whether cell value is ");
      printf("string literal or not, just returning whatever was found ");
      printf("in the xml file\n");
    }
    begin = strinside(begin, "<v>", "</v>");
    if (strcmp(ss, "s") == 0)
      strcpy(value, findss(xl, atoi(begin)));
    else
      strcpy(value, begin);
    free(begin);
    free(ss);
    return value;
  }

  printf("No value in cell %s, returning NULL.\n", s);
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
  char line[LENGTH/100];
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
    printf("Error in function findss:\n");
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
  int i, sheet = 0;
  char *temp;

  /* sheets.txt is a file that is created in the bash script before C is run. It
     uses a simple awk program to list the sheets.*/

  snprintf(sname, sizeof sname, "%s%s", xl->dirname, "/sheets.txt");
  if ((fp = fopen(sname, "r")) == NULL) {
    printf("Error in function setsheetnames:\n");
    perror(sname);
    exit(1);
  }
  while (fgets(line, BUFSIZ, fp) != NULL) {
    i = 0;
    // Remove the '\n' character at the end of the sheet name.
    while (line[i++] != '\n')
      ;
    line[i-1] = '\0';
    /* set memory aside for xl->sheetname to point at. This will stay "alive"
       until the end of the program. */
    temp = (char *)malloc(sizeof(line));
    strcpy(temp, line);
    *(xl->sheetname + sheet) = temp;
    sheet++;
  }
  // final pointer is just a null pointer
  *(xl->sheetname + sheet) = NULL;
  fclose(fp);
}

FILE *opensheet(XLfile *xl, char *sheet) {
  char sname[BUFSIZ];
  FILE *fp;
  int check;
  int i = 0;
  
  while (*(xl->sheetname + i) != NULL && (check = strcmp(sheet, *(xl->sheetname + i)) != 0))
    i++;
  if (check) {
    printf("warning in function opensheet: the given sheet \"%s\" ", sheet);
    printf("does not match any of the known sheets:\n");
    i = 0;
    while ( *(xl->sheetname + i) != NULL)
      printf("%s\n", *(xl->sheetname + i++));
  }
  snprintf(sname, sizeof sname, "%s%s%s%s", xl->dirname, "/", sheet, ".txt");
  if ((fp = fopen(sname, "r")) == NULL) {
    printf("Error in function setsheetnames:\n");
    perror(sname);
    exit(1);
  }
  return fp;
}
