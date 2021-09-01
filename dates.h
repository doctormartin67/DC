#ifndef DATES
#define DATES

#include "libraryheader.h"

#define MINDATE3(X, Y, Z) MINDATE2(MINDATE2(X, Y), Z)
#define MINDATE2(X, Y) minDate(X, Y)
/*
 * As of writing this program, the days since 00/00/1900 in excel is around
 * 45000, so it will take a long time until we are at MAXDAYS. I took 2^17
 */
enum {MAXDAYS = 131072};
enum months {JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC};

extern const unsigned commondays[DEC + 1];
extern const unsigned leapdays[DEC + 1];

typedef struct date {
	unsigned XLday;
	unsigned year;
	unsigned month;
	unsigned day;
} Date;

void setdate(Date date[static restrict 1]);
Date *newDate(unsigned XLday, unsigned year, unsigned month, unsigned day);
Date *minDate(Date *, Date *);
void printDate(const Date *restrict d);

/*
 * inline functions
 */
inline int isleapyear(unsigned year)
{
	if (!(year % 4 == 0))
		return 0;
	else if (!(year % 25 == 0))
		return 1;
	else if (!(year % 16 == 0))
		return 0;
	else return 1;
}

inline unsigned days_in_month_year(unsigned year, unsigned month)
{
	return (isleapyear(year) ? leapdays[month] : commondays[month]);
}

/*
 * Calculate the time in years between two dates
 * m is the amount of months to subtract (usually 0 or 1)
 */
inline double calcyears(const Date d1[static 1], const Date d2[static 1],
		int m)
{
	register double y1 = d1->year;
	register double y2 = d2->year;
	register double m1 = d1->month;
	register double m2 = d2->month;

	return y2 - y1 + (m2 - m1 - m)/12;
}

inline Date *Datedup(const Date *restrict d)
{
	if (!d) return 0;

	Date *date = jalloc(1, sizeof(*date));

	date->XLday = d->XLday;
	date->year = d->year;
	date->month = d->month;
	date->day = d->day;

	return date;
}

#endif
