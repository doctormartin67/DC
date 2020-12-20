#ifndef ACTUARIALFUNCTIONS
#define ACTUARIALFUNCTIONS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lifetables.h"
#include "DCProgram.h"

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
double npx(char *lt, double ageX, double ageXn, int corr);
// nEx is a factor used to give the present value of an amount, taking death chance into
// account. 
double nEx(char *lt, double i, double charge, double ageX, double ageXn, int corr);
// axn is the annuity factor that calculates the present value of investing 1 dollar
// each year from ageX until ageXn divided up by the term, where term is represented in months,
// so term = 12 means we pay 1/12 per months, term = 6 means we pay 2/12 every 2 months.
// prepost determines whether we pay straight away (prepost = 0) or after the first term ends
// (prepost = 1). 
double axn(char *lt, double i, double charge, int prepost, int term,
	   double ageX, double ageXn, int corr);

// discounted yearly payments in case of death
double Ax1n(char *lt, double i, double charge, double ageX, double ageXn,
	    int corr);

// discounted yearly payments in case of death where the payments are cummulative
// (1 + 2 + 3 + ... + n instead of 1 + 1 + 1 + ... + 1)
double IAx1n(char *lt, double i, double charge, double ageX, double ageXn,
	    int corr);

double Iaxn(char *lt, double i, double charge, int prepost, int term,
	   double ageX, double ageXn, int corr);

// Update the current iteration (k) of death lump sum, employer-employee, generation
void evolCAPDTH(CurrentMember *cm, int EREE, int gen, int k);

/* Update the current iteration (k) of Reserves and profit sharing reserves, 
   employer-employee, generation */
void evolRES(CurrentMember *cm, int EREE, int gen, int k);

// calculate the capital life (lump sum) given reserves and contributions
double calcCAP(CurrentMember *cm, int EREE, int gen, int k,
	       double res, double prem, double capdth,
	       double age, double RA, char *lt);

// calculate the reserves given capital life (lump sum) and contributions
double calcRES(CurrentMember *cm, int EREE, int gen, int k,
	       double cap, double prem, double capdth,
	       double age, char *lt);

#endif
