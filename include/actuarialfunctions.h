#ifndef ACTUARIALFUNCTIONS_H
#define ACTUARIALFUNCTIONS_H
#include <math.h>
#include "common.h"
#include "lifetables.h"

#define EPS 0.0000001

extern unsigned long lx[LT_AMOUNT][MAXAGE];

/*
 * Definitions:
 * - lt is the name of a life table that gets put in a hashtable to be called.
 * Within the hash table element there is an int array with lx values. 
 * These lx values define the amount of people alive at a certain age (the 
 * array index).
 * - ageX is the age to start.
 * - ageXn is the age to finish at.
 * - corr is a correction that can be made with age. For example corr = -5 
 * would mean ageX-=5 and ageXn-=5
 * - i is the discount rate.
 * - charge is a possible charge that an insurance company or another entity 
 * might charge on the discount rate.
 */

/*
 * discounted yearly payments in case of death
 */
double Ax1n(register unsigned lt, register double i, register double charge,
		register double ageX, register double ageXn,
		register int corr) PURE ;

/*
 * discounted yearly payments in case of death where the payments are 
 * cummulative (1 + 2 + 3 + ... + n instead of 1 + 1 + 1 + ... + 1)
 */
double IAx1n(register unsigned lt, register double i, register double charge,
		register double ageX, register double ageXn,
		register int corr) PURE ;

double Iaxn(register unsigned lt, register double i, register double charge,
		register unsigned prepost, register unsigned term,
		register double ageX, register double ageXn,
		register int corr) PURE ;

/*
 * inline functions
 */
/* 
 * npx is the chance for some of age ageX to live until the age of ageXn
 * generally the rule is npx = lx(ageXn)/lx(ageX) but because lx has an integer
 * value as its input we need to interpolate both these values
 */
inline double npx(register unsigned lt, register double ageX,
		register double ageXn, register int corr)
{
	register double ip1 = 0.0;
	register double ip2 = 0.0;
	register double max = 0.0;
	register unsigned long lxX = 0;
	register unsigned long lxX1 = 0;
	register unsigned long lxXn = 0;
	register unsigned long lxXn1 = 0;
	ageX += corr;
	ageXn += corr;
	max = MAXAGE - 2;
	ageX = MIN(max, MAX(0, ageX));
	ageXn = MIN(max, MAX(0, ageXn));
	lxX = lx[lt][(unsigned)ageX];
	lxX1 = lx[lt][(unsigned)(ageX + 1)];
	lxXn = lx[lt][(unsigned)ageXn];
	lxXn1 = lx[lt][(unsigned)(ageXn + 1)];

	ip1 = lxX - (ageX - floor(ageX)) * (lxX - lxX1);
	ip2 = lxXn - (ageXn - floor(ageXn)) * (lxXn - lxXn1);

	return ip2/ip1;
}

/* 
 * nEx is a factor used to give the present value of an amount, taking death
 * chance into account. 
 */
inline double nEx(register unsigned lt, register double i, register double charge, 
		register double ageX, register double ageXn, register int corr)
{
	register double im = (1 + i)/(1 + charge) - 1;
	register double n = ageXn - ageX;
	register double vn = 1 / pow(1 + im, n);
	register double nPx = npx(lt, ageX, ageXn, corr);
	return vn * nPx;
}

/*
 * calculates the annuity from ageX to ageXn given the term, f.e. monthly,
 * with a given life table lt, an interest rate i, a cost on the interest rate
 * charge, whether the payment are pre- or post numerando and with an age
 * correction. 
 * The function saves the calculated values in a hashtable so that
 * values already calculated can be searched in the table.
 */
inline double axn(register unsigned lt, register double i, register double charge, 
		register unsigned prepost, register unsigned term,
		register double ageX, register double ageXn, register int corr)
{
	register int payments = 0;
	register double ageXk = 0.0;
	register double value = 0.0;
	register double termfrac = 0.0;

	if (ageX > ageXn + EPS) {
		value = 0;
	} else {
		termfrac = 1.0 / term;
		ageXk = ageX + (double)prepost/term;
		payments = (ageXn - ageX) * term + EPS;
		while (payments--) {
			value += nEx(lt, i, charge, ageX, ageXk, corr);
			ageXk += termfrac;
		}
		/* 
		 * There is a final portion that needs to be added when
		 * the amount of payments don't consider fractions of
		 * age, for example if ageX = 40 and ageXn = 40,5 and
		 * term = 1. This would give 0 payments but we still
		 * need to add nEx/2 in this case. We also need to
		 * subtract one term from ageXk because the while loop
		 * adds one too many.
		 */    
		ageXk -= termfrac * prepost;
		value /= term;
		value += (ageXn - ageXk) * nEx(lt, i, charge, ageX,
					((int)(ageXn*term + EPS))/term
					+ term*prepost, corr);

	}

	return value;
}

inline double CAP_UKMS_UKZT(double res, double prem, double deltacap, double age,
		double RA, double ac, double Ex, double ax)
{
	return (res + prem * (1 - ac) * ax) / Ex + deltacap * (RA - age) * 12;
}

inline double CAP_UKMT(double res, double prem, double capdth, double ac,
		double Ex, double ax, double axcost, double Ax1, double IAx1,
		double Iax, double cKO)
{
	prem *= (1 - ac);
	return (res + prem * ax - capdth * (Ax1 + cKO * axcost)
			- prem * (IAx1 + cKO * Iax)) / Ex;
}

inline double CAP_MIXED(double res, double prem, double ac, double Ex,
		double ax, double axcost, double Ax1, double x10,
		double MIXEDPS, double cKO)
{
	return (res + prem * (1 - ac) * ax)
		/ (Ex + 1.0/x10 * MIXEDPS * (Ax1 + cKO * axcost));
}

#endif
