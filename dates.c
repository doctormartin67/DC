/*
 * dates in excel are given as an integer since 00/00/1900. This file
 * implements functions to create new dates as date struct.
 */
#include <stdio.h>
#include <stdlib.h> /* malloc and free */
#include <stdarg.h>
#include "dates.h"
#include "errorexit.h"

/*
 * inline functions
 */
int isleapyear(unsigned year);
unsigned days_in_month_year(unsigned year, unsigned month);
double calcyears(const struct date d1[static 1],
		const struct date d2[static 1], int m);
struct date *Datedup(const struct date *restrict d);

const unsigned commondays[DEC + 1] = {0, 
	[JAN] = 31, 
	[FEB] = 28, 
	[MAR] = 31, 
	[APR] = 30, 
	[MAY] = 31, 
	[JUN] = 30, 
	[JUL] = 31, 
	[AUG] = 31, 
	[SEP] = 30, 
	[OCT] = 31, 
	[NOV] = 30, 
	[DEC] = 31
};

const unsigned leapdays[DEC + 1] = {1, 
	[JAN] = 31, 
	[FEB] = 29, 
	[MAR] = 31, 
	[APR] = 30, 
	[MAY] = 31, 
	[JUN] = 30, 
	[JUL] = 31, 
	[AUG] = 31, 
	[SEP] = 30, 
	[OCT] = 31, 
	[NOV] = 30, 
	[DEC] = 31
};

/* 
 * if XLday is 0 then this will create a date with the given day, month and 
 * year. Otherwise it will create it with the given XLday.
 */
struct date *newDate(unsigned XLday,
		unsigned year, unsigned month, unsigned day)
{
	struct date *d = jalloc(1, sizeof(*d));

	if (0 == XLday) {
		if (day > days_in_month_year(year, month)) {
			month++;
			day = 1;
		}
		if (month > DEC) {
			year++;
			month = JAN;
		}
		d->day = day;
		d->month = month;
		d->year = year;
	} else {
		d->XLday = XLday;
		setdate(d);
	}

	// Error checking
	if (d->month < 1 || d->month > DEC || d->day < 1
		|| (d->day > days_in_month_year(d->year, d->month))) {
		free(d);
		d = 0;
	}

	return d;
}

/*
 * set day, month and year of the given date using the XLday provided in the
 * excel file. The function uses static arrays to save dates so we don't have
 * to recalculate the same date each time.
 */
void setdate(struct date d[static restrict 1])
{
	if (d->XLday > MAXDAYS - 1)
		die("XLday exceeds the maximum amount of days since "
				"00/00/1900 (%d). MAXDAYS constant needs "
				"increased within code", MAXDAYS);

	static unsigned daytoday[MAXDAYS];
	static unsigned daytomonth[MAXDAYS];
	static unsigned daytoyear[MAXDAYS];

	register unsigned countday = 1;
	register unsigned countyear = 1900;
	register unsigned currentmonth = 0;

	if (!(0 == daytoyear[d->XLday]) 
			&& !(0 == daytomonth[d->XLday])
			&& !(0 == daytoday[d->XLday])) {
		d->day = daytoday[d->XLday];
		d->month = daytomonth[d->XLday];
		d->year = daytoyear[d->XLday];	
	} else {
		while (countday < d->XLday) {
			currentmonth %= 12;
			currentmonth++;

			if (isleapyear(countyear))
				countday += leapdays[currentmonth];
			else
				countday += commondays[currentmonth];

			if (currentmonth == DEC && countday < d->XLday)
				countyear++;
		}
		if (isleapyear(countyear))
			countday -= leapdays[currentmonth];
		else
			countday -= commondays[currentmonth];

		daytoday[d->XLday] = d->XLday - countday;
		daytomonth[d->XLday] = currentmonth;
		daytoyear[d->XLday] = countyear;

		d->day = daytoday[d->XLday]; 
		d->month = daytomonth[d->XLday];
		d->year = daytoyear[d->XLday];
	}
}


/*
 * return the minimum date
 */
struct date *minDate(struct date *d1, struct date *d2)
{
	if (0 == d1 || 0 == d2) return 0;

	struct date *min = d2;

	if (d1->year < d2->year) {
		min = d1;
	} else if (d1->year == d2->year) {
		if (d1->month < d2->month) {
			min = d1;
		} else if (d1->month == d2->month) {
			if (d1->day < d2->day)
				min = d1;
		}
	}

	return min;
}

void printDate(const struct date *restrict d)
{
	if (d)
		printf("%d/%d/%d\n", d->day, d->month, d->year);
	else
		printf("(null)\n");
}
