#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include "libraryheader.h"

// s is the name of the excel file to set the values of
void setXLvals(XLfile *xl, const char *s) {
    char temp[MAXLENGTH];
    char *pt = temp;

    strcpy(temp, s);
    strcpy(xl->fname, temp);
    if ((pt = strstr(temp, ".xls")) == NULL || !FILEexists(temp)) {// not an excel file
	printf("Please select an valid excel file.\n");
	printf("Exiting program.\n");
	exit(0);
    }
    *pt = '\0';
    strcpy(xl->dirname, temp);
    setsheetnames(xl);
}

/* fp is an open sheet, usually the sheet containing the data to evaluate 
   s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
 */
char *cell(FILE *fp, const char *s, XLfile *xl) {
    char line[BUFSIZ];
    char *value; // value of cell to return (string)

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
	*(xl->sheetname + sheet++) = strdup(line);
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
    char *npt = next;
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


char *valueincell(XLfile *xl, char *line, const char *find) {
    char *begin;
    char *ss; // string to determine whether I need to call findss or not
    char *value; // value of cell to return (string)
    char *temp; // used to free allocated memory after findss is called
    unsigned int i = 0, j = 0;

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
    unsigned int countday = 1;
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

	if ((date->day = daytoday[date->XLday]) > 31) {
	    printf("Error in %s: %d is not a valid day.\n", __func__, date->day);
	    exit(1);
	}
	if((date->month = daytomonth[date->XLday]) > 12) {
	    printf("Error in %s: %d is not a valid day.\n", __func__, date->month);
	    exit(1);
	}
	date->year = daytoyear[date->XLday];
    }
}

/* if XLday is 0 then this will create a date with the given day, month and year. Otherwise it
   will create it with the given XLday.*/
Date *newDate(unsigned int XLday, int year, int month, int day) {
    int tday = day;
    int tmonth = month;
    int tyear = year;

    Date *temp = (Date *)malloc(sizeof(Date));

    if (tday > (isleapyear(tyear) ? leapdays[tmonth] : commondays[tmonth])) {
	tmonth++;
	tday = 1;
    }
    if (tmonth > DEC) {
	tyear++;
	tmonth = JAN;
    }

    if (XLday == 0) {
	temp->day = tday;
	temp->month = tmonth;
	temp->year = tyear;
    }
    else {
	temp->XLday = XLday;
	setdate(temp);
    }

    // Error checking
    if (temp->day > (isleapyear(temp->year) ? leapdays[temp->month] : commondays[temp->month])) {
	printf("Error in newDate: %d is not a valid day of month %d\n",
		temp->day, temp->month);
	printf("Exiting program\n");
	exit(1);
    }
    if (temp->month > DEC) {
	printf("Error in newDate: there are no %d months\n", temp->month);
	printf("Exiting program\n");
	exit(1);
    }

    return temp;
}

Date *minDate(int argc, ...) {
    Date *min; // minimum to return
    Date *currmin; // current minimum
    va_list dates;

    va_start(dates, argc);
    min = va_arg(dates, Date *);

    for (int i = 1; i < argc; i++) {
	currmin = va_arg(dates, Date *);
	if (min->year > currmin->year) {
	    min = currmin;
	}
	else if (min->year == currmin->year) {
	    if (min->month > currmin->month) {
		min = currmin;
	    }
	    else if (min->month == currmin->month) {
		if (min->day > currmin->day)
		    min = currmin;
	    }
	}
    }
    // Error checking
    if (min->day > (isleapyear(min->year) ? leapdays[min->month] : commondays[min->month])) {
	printf("Error in newDate: %d is not a valid day of month %d\n",
		min->day, min->month);
	printf("Exiting program\n");
	exit(1);
    }
    if (min->month > DEC) {
	printf("Error in newDate: there are no %d months\n", min->month);
	printf("Exiting program\n");
	exit(1);
    }

    return min;
}

// Calculate the time in years between two dates
// m is the amount of months to subtract (usually 0 or 1)
double calcyears(Date *d1, Date *d2, int m) {
    return d2->year - d1->year +
	(double)(d2->month - d1->month - m)/12;
}

void printDate(Date *d) {
    printf("%d/%d/%d\n", d->day, d->month, d->year);
}
