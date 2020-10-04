#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "libraryheader.h"

#define TEMPPATH "/home/doctormartin67/Projects/library/temp/" /*This should be updated, because it's too dependant
					    on the library name */
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
  char line[BUFSIZ];
  char sname[BUFSIZ/4]; // name of the sheet to open (xml file)
  char *begin;
  char *ss; // string to determine whether I need to call findss or not
  int sheetnr;
  struct stat xlbuf;
  struct stat DMbuf;
  static char value[BUFSIZ]; // value of cell to return (string)
  char *t;
  char *end;
  char command[BUFSIZ];

  /* awk is used to create a text file with all the cell values printed per line.
     createDMfile will check whether that text file has been created and also when
     it was created and will create it if neccessary and assign sname with the correct
     name of the file. */
  createDMfile(sname, xl, sheet);
  // TODO: change this so that it doesnt open and close the FILE everytime cell is called
  if ((fp = fopen(sname, "r")) == NULL) {
    perror("DM.txt");
    exit(1);
  }
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
    fclose(fp);
    return value;
  }

  fclose(fp);
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
  char *temp;
  char *temp1; // used to replace "&" with ""
  char *temp2; // used to replace ";" with ""
  int sheet = 0;

  // create the name of the xml file to find the sheet names
  snprintf(sname, sizeof sname, "%s%s", xl->dirname, "/xl/workbook.xml");
  if ((fp = fopen(sname, "r")) == NULL) {
    perror(sname);
    exit(1);
  }
  while (fgets(line, BUFSIZ, fp) != NULL) {
    begin = line;
    while ((begin = strstr(begin, "<sheet name=")) != NULL) {
      temp = strinside(begin, "<sheet name=\"", "\" sheetId");
      temp1 = replace(temp, "&", "");
      temp2 = replace(temp1, ";", "");
      *(xl->sheetname + sheet) = temp2;
      sheet++;
      begin += strlen("<sheet name=");
      free(temp);
      free(temp1);
      // we don't free(temp2) because we need it "alive" for xl to point to
    }
  }
  // final pointer is just a null pointer
  *(xl->sheetname + sheet) = NULL;
  fclose(fp);
}

/* this function creates the DM file if its not already been created of if it's outdated and also
   sets the fawk of the DM file
*/
void createDMfile(char *fawk, XLfile *xl, char *sheet) {
  int sheetnr;
  struct stat xlbuf;
  struct stat DMbuf;
  char *s, *t;
  char command[BUFSIZ];

  if(!(sheetnr = findsheetID(xl, sheet)))
    exit(0);
  // create the name of the xml file to find the cell value
  snprintf(fawk, BUFSIZ, "%s%s%d%s", xl->dirname, "/xl/worksheets/sheet", sheetnr, ".xml");
  // we need to replace spaces with "\ " for the command
  t = replace(fawk, " ", "\\ ");

  // this is used to retrieve st_mtime
  if (stat(fawk, &xlbuf) < 0) {
    perror(fawk);
    exit(1);
  }
  memset(fawk, '\0', BUFSIZ);
  snprintf(fawk, BUFSIZ, "%s%s%s%s", TEMPPATH, "DM", sheet, ".txt");
  s = replace(fawk, " ", "\\ ");
  if(!FILEexists(fawk)) {
    snprintf(command, sizeof(command), "%s%s%s%s", "DM ", t, " > ", s);
    system(command);
  }
  if (stat(fawk, &DMbuf) < 0) {
    perror(fawk);
    exit(1);
  }
  if (xlbuf.st_mtime > DMbuf.st_mtime) {
    snprintf(command, sizeof(command), "%s%s%s%s", "DM ", t, " > ", s);
    system(command);
  }
  free(t);
}
