#ifndef ACTUARIALFUNCTIONS
#define ACTUARIALFUNCTIONS
#include <math.h>
#include "lifetables.h"
#include "DCProgram.h"

enum {DBO, NC, IC, ASSETS};
enum {DEF, IMM}; // deferred or immediate payment

typedef struct {
	LifeTable *lt;
	double res;
	double prem; 
	double deltacap; 
	double capdth;
	double age; 
	double RA; 
	double cap;
} CalcInput;

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
 * axn is the annuity factor that calculates the present value of investing 1
 * dollar each year from ageX until ageXn divided up by the term, where term is
 * represented in months, so term = 12 means we pay 1/12 per months, term = 6
 * means we pay 2/12 every 2 months. prepost determines whether we pay straight
 * away (prepost = 0) or after the first term ends (prepost = 1). 
 */
double axn(register unsigned lt, register double i, register double charge,
		register unsigned prepost, register unsigned term,
		register double ageX, register double ageXn,
		register int corr) PURE ;

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

void evolCAPDTH(CurrentMember *restrict cm, int k);
void evolRES(CurrentMember *restrict cm, int k);
void evolPremiums(CurrentMember *restrict cm, int k);
void evolART24(CurrentMember *restrict cm, int k);
double calcCAP(const CurrentMember *restrict cm, const CalcInput *restrict);
double calcRES(const CurrentMember *restrict cm, const CalcInput *restrict);
void evolDBONCIC(CurrentMember *restrict cm, int k,
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT]);
void evolEBP(CurrentMember *restrict cm, int k,
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT]);

// This is used as a help function to retrieve the appropriate amount
// for the formula
double getamount(const CurrentMember *restrict cm, int k, unsigned DBONCICASS,
		unsigned method, unsigned assets, unsigned DEFIMM,
		unsigned PBOTBO, 
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT], 
		const double REDCAPTOT[const static METHOD_AMOUNT]);

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
	register double ip1;
	register double ip2;
	register int lxX;
	register int lxX1;
	register int lxXn;
	register int lxXn1;
	ageX += corr;
	ageXn += corr;
	ageX = MAX2(0, ageX);
	ageXn = MAX2(0, ageXn);
	lxX = lx(lt, ageX);
	lxX1 = lx(lt, ageX + 1);
	lxXn = lx(lt, ageXn);
	lxXn1 = lx(lt, ageXn + 1);

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
	register double im = (1+i)/(1+charge) - 1;
	register double n = ageXn - ageX;
	register double vn = 1/pow(1 + im, n);
	register double nPx = npx(lt, ageX, ageXn, corr);
	return vn * nPx;
}

#endif
