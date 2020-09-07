#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "libraryheader.h"

#define LENGTH BUFSIZ * 1000

static struct stat buf;

char *trim(char *s) {
  char *t;
  t = s;
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'){
    t = ++s;
  }
  while (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\r'){
    s++;
  }
  *s = '\0';
  return t;
}

// create a zip file of an excel document and extract
void createXLzip(char *s) {
  char *t;
  char *end;
  char command[BUFSIZ];
  t = replace(s, " ", "\\ ");
  s = replace(s, " ", "\\ ");
  if ((end = strstr(t, ".xls")) == NULL) {// not an excel file
    printf("Please select an valid excel file.\n");
    printf("Exiting program.\n");
    exit(0);
  }
  *end = '\0';
  snprintf(command, sizeof command, "%s%s%s%s%s", "cp ", s, " ", t, ".zip");
  system(command);

  // *******unzip zip file*******
  
  // make directory for all files
  snprintf(command, sizeof command, "%s%s", "mkdir ", t);
  if (!DIRexists(replace(t, "\\ ", " ")))
    system(command);
  else {
    snprintf(command, sizeof command, "%s%s", "rm -rf ", t);
    system(command);
  }
  // unzip in directory
  snprintf(command, sizeof command, "%s%s%s%s", "unzip -q ", t, ".zip -d ", t);
  if(system(command)) {
    printf(">>>>>>");
    printf("THE EXCEL FILE YOU ARE TRYING TO OPEN HAS A PASSWORD.\n");
    printf(">>>>>>");
    printf("REMOVE THE PASSWORD FROM THE FILE AND TRY AGAIN.\n");
    exit(1);
  }
  free(t);
  free(s);
}

/* s is the name of the cell to retrieve value (for example B11).
   fname is the name of the excel file with xls* extention.
   sheet is the number of the sheet to open,
*/
char *cell(char *s, char *fname, int sheet) {
  
  FILE *fp;
  char line[LENGTH];
  char sname[BUFSIZ]; // name of the sheet to open (xml file)
  char *begin;
  char *end;
  long int mtime_XLfile; // time excel file was last modified
  long int mtime_XLfolder; // time excel folder was last modified
  static char value[BUFSIZ]; // value of cell to return (string)

  if (stat(fname, &buf) < 0) {
    perror(fname);
    exit(1);
  }
  
  mtime_XLfile = buf.st_mtime;

  // here we remove the extension of the excel file from fname
  if ((end = strstr(fname, ".xls")) == NULL) {// not an excel file
    printf("Please select an valid excel file.\n");
    printf("Exiting program.\n");
    exit(0);
  }
  
  *end = '\0';

  if(!DIRexists(fname)) {
    *end = '.';
    createXLzip(fname);
    *end = '\0';
  }
  
  if (stat(fname, &buf) < 0) {
    perror(fname);
    exit(1);
  }
  
  mtime_XLfolder = buf.st_mtime;

  if (mtime_XLfile > mtime_XLfolder) {
    *end = '.';
    createXLzip(fname);
    *end = '\0';
  }
  
  snprintf(sname, sizeof sname, "%s%s%d%s", fname, "/xl/worksheets/sheet", sheet, ".xml");

  if ((fp = fopen(sname, "r")) == NULL) {
    perror(sname);
    exit(1);
  }
  while (fgets(line, LENGTH, fp) != NULL) {
    if ((begin = strstr(line, s)) == NULL)
      continue;
    begin = strstr(begin, "<v>");
    begin += 3;
    end = strstr(begin, "</v>");
    *end = '\0';
    strcpy(value, begin);
    *end = '<';
    return value;
  }

  fclose(fp);
  printf("No value in cell %s, returning 0\n", s);
  return "0";
}

// replace all occurences of string oldW with newW in s
char *replace(const char *s, const char *oldW, 
	      const char *newW) 
{ 
  char *result; 
  int i, cnt = 0; 
  int newWlen = strlen(newW); 
  int oldWlen = strlen(oldW); 
  
  // Counting the number of times old word 
  // occur in the string 
  for (i = 0; s[i] != '\0'; i++) { 
    if (strstr(&s[i], oldW) == &s[i]) { 
      cnt++; 
  
      // Jumping to index after the old word. 
      i += oldWlen - 1; 
    } 
  } 
  
  // Making new string of enough length 
  result = (char *) malloc(i + cnt * (newWlen - oldWlen) + 1); 
  
  i = 0; 
  while (*s) { 
    // compare the substring with the result 
    if (strstr(s, oldW) == s) { 
      strcpy(&result[i], newW); 
      i += newWlen; 
      s += oldWlen; 
    } 
    else
      result[i++] = *s++; 
  } 
  
  result[i] = '\0'; 
  return result; 
}

// Check if file exists
int FILEexists(const char *fname)
{
  FILE *file;
  if ((file = fopen(fname, "r")))
    {
      fclose(file);
      return 1;
    }
  return 0;
}

// Check if DIR exists
int DIRexists(const char *dname) {
  DIR* dir = opendir(dname);
  if (dir) {
    /* Directory FILEexists. */
    return 1;
    closedir(dir);
  } else if (ENOENT == errno) {
    /* Directory does not exist. */
    return 0;
  } else {
    /* opendir() failed for some other reason. */
    return -1;
  }
}
