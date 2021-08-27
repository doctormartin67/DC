#ifndef DATES
#define DATES

/*
 * As of writing this program, the days since 00/00/1900 in excel is around
 * 45000, so it will take a long time until we are at MAXDAYS. I took 2^17
 */
enum {MAXDAYS = 131072};

enum months {JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC};
typedef struct date {
	unsigned XLday; // excel starts counting at 31/12/1899
	int year;
	int month;
	int day;
} Date;
int isleapyear(int year);
void setdate(Date *date);
Date *newDate(unsigned XLday, int year, int month, int day);
int days_in_month_year(int year, int month);
Date *minDate(int, ...);
double calcyears(const Date *d1, const Date *d2, int m);
void printDate(Date *d);

#endif
