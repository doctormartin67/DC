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
