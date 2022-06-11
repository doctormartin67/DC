#ifndef ASSUMPTIONS
#define ASSUMPTIONS

#include "userinterface.h"

typedef struct {
	int agecorr;
	unsigned incrSalk0;
	unsigned incrSalk1;
	unsigned method;
	double infl;
	double DR;
	double DR113;
	double TRM_PercDef;
	double taxes;
	struct date *DOC;
} Assumptions;

Assumptions ass;

void init_builtin_vars(void);
void setassumptions(void);
void set_tariffs(const CurrentMember *cm);
void setparameters(const CurrentMember *cm, int k);
double salaryscale(const CurrentMember *cm, int k);
double calcA(const CurrentMember *cm, int k);
double calcC(const CurrentMember *cm, int k);
double calcDTH(const CurrentMember *cm, int k);
double NRA(const CurrentMember *cm, int k);
double wxdef(const CurrentMember *cm, int k);
double retx(const CurrentMember *cm, int k);

#endif
