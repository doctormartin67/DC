/*
 * dates in excel are given as an integer since 00/00/1900. This file
 * implements functions to create new dates as Date struct.
 */
#include <stdio.h>
#include <stdlib.h> /* malloc and free */
#include <stdarg.h>
#include "libraryheader.h"
#include "dates.h"
#include "errorexit.h"

static const int commondays[] = {0, 
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

static const int leapdays[] = {1, 
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

int isleapyear(int year)
{
	if (!(year % 4 == 0))
		return 0;
	else if (!(year % 25 == 0))
		return 1;
	else if (!(year % 16 == 0))
		return 0;
	else return 1;
}

/* 
 * if XLday is 0 then this will create a date with the given day, month and 
 * year. Otherwise it will create it with the given XLday.
 */
Date *newDate(unsigned XLday, int year, int month, int day)
{
	Date *date = jalloc(1, sizeof(Date));

	if (0 == XLday) {
		if (day > days_in_month_year(year, month)) {
			month++;
			day = 1;
		}
		if (month > DEC) {
			year++;
			month = JAN;
		}
		date->day = day;
		date->month = month;
		date->year = year;
	} else {
		date->XLday = XLday;
		setdate(date);
	}

	// Error checking
	if (date->day > days_in_month_year(date->year, date->month)
			|| date->day < 1 || date->day > 31
			|| date->month < 1 || date->month > DEC
			|| date->year < 0) {
		free(date);
		date = 0;
	}

	return date;
}

/*
 * set day, month and year of the given date using the XLday provided in the
 * excel file. The function uses static arrays to save dates so we don't have
 * to recalculate the same date each time.
 */
void setdate(Date *date)
{
	if (date->XLday > MAXDAYS - 1)
		errExit("[%s] XLday exceeds the maximum amount of days since "
				"00/00/1900 (%d). MAXDAYS constant needs "
				"increased within code\n", __func__, MAXDAYS);

	static unsigned daytoday[MAXDAYS];
	static unsigned daytomonth[MAXDAYS];
	static unsigned daytoyear[MAXDAYS];

	unsigned int countday = 1;
	int countyear = 1900;
	int currentmonth = 0;

	if (!(0 == daytoyear[date->XLday]) 
			&& !(0 == daytomonth[date->XLday])
			&& !(0 == daytoday[date->XLday])) {
		date->day = daytoday[date->XLday];
		date->month = daytomonth[date->XLday];
		date->year = daytoyear[date->XLday];	
	} else {
		while (countday < date->XLday) {
			currentmonth %= 12;
			currentmonth++;

			if (isleapyear(countyear))
				countday += leapdays[currentmonth];
			else
				countday += commondays[currentmonth];

			if (currentmonth == DEC && countday < date->XLday)
				countyear++;
		}
		if (isleapyear(countyear))
			countday -= leapdays[currentmonth];
		else
			countday -= commondays[currentmonth];

		daytoday[date->XLday] = date->XLday - countday;
		daytomonth[date->XLday] = currentmonth;
		daytoyear[date->XLday] = countyear;

		date->day = daytoday[date->XLday]; 
		date->month = daytomonth[date->XLday];
		date->year = daytoyear[date->XLday];
	}
}

int days_in_month_year(int year, int month)
{
	return (isleapyear(year) ? leapdays[month] : commondays[month]);
}

/*
 * return the minimum date given a list of dates
 */
Date *minDate(int argc, ...)
{
	Date *min = 0;
	Date *currmin = 0;
	va_list dates;

	va_start(dates, argc);
	min = va_arg(dates, Date *);

	for (int i = 1; i < argc; i++) {
		currmin = va_arg(dates, Date *);
		if (min->year > currmin->year) {
			min = currmin;
		} else if (min->year == currmin->year) {
			if (min->month > currmin->month) {
				min = currmin;
			} else if (min->month == currmin->month) {
				if (min->day > currmin->day)
					min = currmin;
			}
		}
	}

	if (min->year < 0) {
		errExit("[%s] invalid year [%d]\n", __func__, min->year);
	} else if (min->month > DEC || min->month < JAN) {
		errExit("[%s] invalid month [%d]\n", __func__, min->month);
	} else if (min->day > (isleapyear(min->year) ? leapdays[min->month]
				: commondays[min->month])) {
		errExit("[%s] invalid day [%d]\n", __func__, min->day);
	}

	va_end(dates);

	return min;
}

/*
 * Calculate the time in years between two dates
 * m is the amount of months to subtract (usually 0 or 1)
 */
double calcyears(const Date *d1, const Date *d2, int m)
{
	return d2->year - d1->year + (double)(d2->month - d1->month - m)/12;
}

void printDate(Date *d)
{
	if (d)
		printf("%d/%d/%d\n", d->day, d->month, d->year);
	else
		printf("(null)\n");
}

