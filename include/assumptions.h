#ifndef ASSUMPTIONS
#define ASSUMPTIONS

#include "userinterface.h"

enum {
	IA_CALCA, IA_CALCC, IA_CALCDTH, IA_SS, IA_NRA, IA_WXDEF, IA_RETX,
	IA_AMOUNT
};

/*
 * The assumptions for an IAS19 evaluation:
 * - DOC = Date of Calculation. this will point to the date that is given in
 *   the UI
 * - infl = inflation
 * - DR = discount rate 
 * - DR113 = discount rate according to $113 of IAS19
 * - agecorr = age correction. The life tables in Belgium are outdated,
 *   therefore to have a more realistic assumption about chance of death a
 *   correction is made on the age of the affilate
 * - ct is an array for assumptions that will use the interpreter to find the
 *   amount depending on the variables given. Each of the assumptions has it's
 *   own casetree given in the UI
 * The following element won't usually change from year to year:
 * - incrSalk0 is just a boolean to determine whether the salary should be
 *   increased at k = 0
 * - incrSalk1 is just a boolean to determine whether the salary should be
 *   increased at k = 1
 * - TRM_PercDef is the percentage of deferred members that will keep their
 *   reserves with the current employer at termination (usually equals 1)
 * - method is the methodology which used bits
 * - taxes are usually 0.1326 (0.0886 + 0.044) for IAS, 0 for FAS
 */
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
	const char *ss;
	const char *nra;
	const char *wxdef;
	const char *retx;
	const char *calc_a;
	const char *calc_c;
	const char *calc_dth;
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
