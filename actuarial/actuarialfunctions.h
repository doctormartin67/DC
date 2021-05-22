#ifndef ACTUARIALFUNCTIONS
#define ACTUARIALFUNCTIONS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lifetables.h"
#include "DCProgram.h"

enum {DBO, NC, IC, ASSETS};
enum {DEF, IMM}; // deferred or immediate payment

/*
Definitions:
- lt is the name of a life table that gets put in a hashtable to be called.
within the hash table element there is a int array with lx values. These lx 
values define the amount of people alive at a certain age (the array index).
- ageX is the age to start.
- ageXn is the age to finish at.
- corr is a correction that can be made with age. For example corr = -5 would mean ageX-=5
and ageXn-=5
- i is the discount rate.
- charge is a possible charge that an insurance company or another entity might charge on
the discount rate.
 */

// npx is the chance for some of age ageX to live until the age of ageXn
double npx(register unsigned int lt, register double ageX, 
	register double ageXn, register int corr) PURE ;
// nEx is a factor used to give the present value of an amount, taking death chance into
// account. 
double nEx(unsigned int lt, double i, double charge, double ageX, double ageXn, int corr) PURE ;
// axn is the annuity factor that calculates the present value of investing 1 dollar
// each year from ageX until ageXn divided up by the term, where term is represented in months,
// so term = 12 means we pay 1/12 per months, term = 6 means we pay 2/12 every 2 months.
// prepost determines whether we pay straight away (prepost = 0) or after the first term ends
// (prepost = 1). 
double axn(unsigned int lt, double i, double charge, int prepost, int term,
	double ageX, double ageXn, int corr) PURE ;

// discounted yearly payments in case of death
double Ax1n(unsigned int lt, double i, double charge, double ageX, double ageXn,
	int corr) PURE ;

// discounted yearly payments in case of death where the payments are cummulative
// (1 + 2 + 3 + ... + n instead of 1 + 1 + 1 + ... + 1)
double IAx1n(unsigned int lt, double i, double charge, double ageX, double ageXn,
	int corr) PURE ;

double Iaxn(unsigned int lt, double i, double charge, int prepost, int term,
	double ageX, double ageXn, int corr) PURE ;

// Update the current iteration (k) of death lump sum, employer-employee, generation
void evolCAPDTH(CurrentMember *cm, int EREE, int gen, int k);

/* Update the current iteration (k) of Reserves and profit sharing reserves, 
   employer-employee, generation */
void evolRES(CurrentMember *cm, int EREE, int gen, int k);

/* Update the current iteration (k) of ART24, 
   employer-employee, generation */
void evolART24(CurrentMember *cm, int k);

// calculate the capital life (lump sum) given reserves and contributions
double calcCAP(CurrentMember *cm, 
	double res, double prem, double deltacap, double capdth,
	double age, double RA, LifeTable *lt);

// calculate the reserves given capital life (lump sum) and contributions
double calcRES(CurrentMember *cm, int k,
	double cap, double prem, double deltacap, double capdth,
	double age, LifeTable *lt);

// update all the DBO's, NC's, IC's and plan assets at iteration k
void evolDBONCIC(CurrentMember *cm, int k, 
	double ART24TOT[], double RESTOT[], double REDCAPTOT[]);

// update EBP cashflows
void evolEBP(CurrentMember *cm, int k, 
	double ART24TOT[], double RESTOT[], double REDCAPTOT[]);

// This is used as a help function to retrieve the appropriate amount
// for the formula
double getamount(CurrentMember *cm,  int k,  
	unsigned short DBONCICASS,  
	unsigned short method,  
	unsigned short assets,  
	unsigned short DEFIMM, 
	unsigned short PBOTBO,
	double ART24TOT[], double RESTOT[], double REDCAPTOT[]); 
#endif
