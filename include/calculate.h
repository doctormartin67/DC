#ifndef CALCULATE_H
#define CALCULATE_H
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

void evolCAPDTH(CurrentMember *restrict cm, int k);
void evolRES(CurrentMember *restrict cm, int k);
void evolPremiums(CurrentMember *restrict cm, int k);
void update_art24(CurrentMember *restrict cm, int k);
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

#endif
