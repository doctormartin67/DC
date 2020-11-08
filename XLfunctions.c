#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
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
  char *begin;
  char *ss; // string to determine whether I need to call findss or not
  char *value; // value of cell to return (string)
  char *temp; // used to free allocated memory after findss is called

  fseek(fp, 0, SEEK_SET);
  while (fgets(line, BUFSIZ, fp) != NULL) {
    if ((value = valueincell(xl, line, s)) == NULL)
      continue;
    return value;
  }
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
  char line[BUFSIZ];
  char sname[BUFSIZ];
  char sindex[BUFSIZ/32]; //used to convert int to string
  char *begin;
  int count = 0;
  char *value; // value of cell to return (string)
  // create the name of the txt file to find the string
  snprintf(sname, sizeof sname, "%s%s", xl->dirname, "/ss.txt");
  snprintf(sindex, sizeof sindex, "%d:\t", index);
  if ((fp = fopen(sname, "r")) == NULL) {
    printf("Error in function findss:\n");
    perror(sname);
    exit(1);
  }
  while (fgets(line, sizeof(line), fp) != NULL) {

    if (strstr(line, sindex) == NULL) {
      count++;
      continue;
    }
    if(count == index) {
      begin = line;
      begin = strinside(begin, "\t", "\n");
      value = (char *)malloc((strlen(begin) + 1) * sizeof(char));
      strcpy(value, begin);
      free(begin);
      fclose(fp);
      return value;
    }
  }
  fclose(fp);
  printf("Sadly, didn't find the correct string, returning NULL.\n");
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

void nextcol(char *next) {
  char *npt;
  int i = 0, j = 0, endindex = 0, finalindex = 0;
  /* endindex is the last index of the letters
     finalindex is the last index of the whole string*/

  // Find the final letter of the column.
  while (!isdigit(*(next + i)) && *(next + i) != '\0') {
    // Check if next is capital letters, if not then exit.
    if (*(next + i) <= 'z' && *(next + i) >= 'a') {
      printf("Error: In excel columns always have capital letters and %s ", next);
      printf("are not all capital letters. Exiting...\n");
      exit(0);
    }
    npt = next + i++;
  }
  endindex = i--; // used to set last letter to A if all others are Z.
  while (*(next + i++))
    ;
  finalindex = i; // used to shift all numbers to fit A at the end of letters
  i = endindex - 1;
  while (*npt == 'Z') {
    if (i-- == 0) {
      while (finalindex - j++ > endindex)
	*(next + finalindex - j + 1) = *(next + finalindex - j);
      *(next + endindex) = 'A';
    }
    *npt-- = 'A';
  }
  if (i >= 0)
    (*npt)++;
}

char *valueincell(XLfile *xl, char *line, char *find) {
  char *begin;
  char *ss; // string to determine whether I need to call findss or not
  char *value; // value of cell to return (string)
  char *temp; // used to free allocated memory after findss is called
  char sname[BUFSIZ/4];
  int i = 0, j = 0;

  memset(sname, '0', sizeof(sname));
  strcpy(sname, find);
  strcat(sname, "\"");

  begin = line;

  // the line should contain the cell at the start
  while (i < strlen(find)) {
    if (begin[i] == find[i])
      j++;
    i++;
  }
  if (i != j)
    return NULL;
  ss = strinside(begin, "t=\"", "\">");
  begin = strinside(begin, "<v>", "</v>");
  if (ss != NULL && strcmp(ss, "s") == 0) {
    temp = findss(xl, atoi(begin));
    value = (char *)malloc((strlen(temp) + 1) * sizeof(char));
    strcpy(value, temp);
    free(temp);
  }
  else {
    value = (char *)malloc((strlen(begin) + 1) * sizeof(char));
    strcpy(value, begin);
  }
  free(begin);
  free(ss);
  return value;
}

//---Date Functionality---

static const int commondays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const int leapdays[] = {1, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int isleapyear(int year) {
  if (!(year%4 == 0))
    return 0;
  else if (!(year%25 == 0))
    return 1;
  else if (!(year%16 == 0))
    return 0;
  else return 1;
}

void setdate(Date *date) {
  static unsigned int daytoday[BUFSIZ * 16]; // We save the searched days in this array
  static unsigned int daytomonth[BUFSIZ * 16]; // We save the searched days in this array
  static unsigned int daytoyear[BUFSIZ * 16]; // We save the searched days in this array
  int countday = 1;
  int countyear = 1900; // excel starts counting at 00/00/1900
  int currentmonth = 0;

  // If the day has already been called and saved, then we just take it from the
  // array where we save it, otherwise calculate it.
  if (!(daytoyear[date->XLday] == 0) && !(daytomonth[date->XLday] == 0)
      && !(daytoday[date->XLday] == 0)) {
    date->day = daytoday[date->XLday];
    date->month = daytomonth[date->XLday];
    date->year = daytoyear[date->XLday];	
  }
  else {
    while (countday < date->XLday) {
      currentmonth = currentmonth%12;
      currentmonth++;

      if (isleapyear(countyear))
	countday+=leapdays[currentmonth];
      else
	countday+=commondays[currentmonth];
    
      if (currentmonth == DEC && countday < date->XLday)
	countyear++;
    }
    if (isleapyear(countyear))
      countday-=leapdays[currentmonth];
    else
      countday-=commondays[currentmonth];
	  
    daytoday[date->XLday] = date->XLday - countday;
    daytomonth[date->XLday] = currentmonth;
    daytoyear[date->XLday] = countyear;

    date->day = daytoday[date->XLday];
    date->month = daytomonth[date->XLday];
    date->year = daytoyear[date->XLday];
  }
}

Date *newDate(int day, int month, int year) {
  Date *temp = (Date *)malloc(sizeof(Date));
  temp->day = day;
  temp->month = month;
  temp->year = year;
  return temp;
}
