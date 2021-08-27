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
 * npx is the chance for some of age ageX to live until the age of ageXn
 */
double npx(register unsigned lt, register double ageX, register double ageXn,
		register int corr) PURE ;

/* 
 * nEx is a factor used to give the present value of an amount, taking death chance into
 * account. 
 */
double nEx(unsigned lt, double i, double charge, double ageX, double ageXn, 
		int corr) PURE ;
/* 
 * axn is the annuity factor that calculates the present value of investing 1
 * dollar each year from ageX until ageXn divided up by the term, where term is
 * represented in months, so term = 12 means we pay 1/12 per months, term = 6
 * means we pay 2/12 every 2 months. prepost determines whether we pay straight
 * away (prepost = 0) or after the first term ends (prepost = 1). 
 */
double axn(unsigned lt, double i, double charge, int prepost, int term,
		double ageX, double ageXn, int corr) PURE ;

/*
 * discounted yearly payments in case of death
 */
double Ax1n(unsigned lt, double i, double charge, double ageX, double ageXn,
		int corr) PURE ;

/*
 * discounted yearly payments in case of death where the payments are 
 * cummulative (1 + 2 + 3 + ... + n instead of 1 + 1 + 1 + ... + 1)
 */
double IAx1n(unsigned lt, double i, double charge, double ageX, double ageXn,
		int corr) PURE ;

double Iaxn(unsigned lt, double i, double charge, int prepost, int term,
		double ageX, double ageXn, int corr) PURE ;

void evolCAPDTH(CurrentMember *cm, int k);
void evolRES(CurrentMember *cm, int k);
void evolPremiums(CurrentMember *cm, int k);
void evolART24(CurrentMember *cm, int k);
double calcCAP(CurrentMember *cm, CalcInput *);
double calcRES(CurrentMember *cm, CalcInput *);
void evolDBONCIC(CurrentMember *cm, int k,
		double ART24TOT[const static METHOD_AMOUNT],
		double RESTOT[const static METHOD_AMOUNT], 
		double REDCAPTOT[const static METHOD_AMOUNT]);
void evolEBP(CurrentMember *cm, int k, 
		double ART24TOT[const static METHOD_AMOUNT],
		double RESTOT[const static METHOD_AMOUNT],
		double REDCAPTOT[const static METHOD_AMOUNT]);

// This is used as a help function to retrieve the appropriate amount
// for the formula
double getamount(const CurrentMember *cm, int k, unsigned DBONCICASS,
		unsigned method, unsigned assets, unsigned DEFIMM,
		unsigned PBOTBO, 
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT], 
		const double REDCAPTOT[const static METHOD_AMOUNT]);
#endif
