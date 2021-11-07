#include <assert.h>
#include <math.h>
#include "assumptions.h"
#include "lifetables.h"

static union value parameters[VAR_AMOUNT];

enum {VAR_INTERPRETER, VAR_FIXED, VAR_COMBO};
static const char *get_var(unsigned ui, unsigned var_type,
		Hashtable ht[static 1]);
static void set_methodology(Hashtable ht[static 1]);
static void replant(struct casetree **ct, Hashtable ht[static 1],
		unsigned it);

void setassumptions(void)
{
	/* this needs updating when we have a UITY!!! */
	Hashtable *htTY = get_user_input(USER_INPUT_LY);
	Hashtable *htLY = get_user_input(USER_INPUT_LY);
	char temp[BUFSIZ];
	char *year, *month, *day;
	year = month = day = 0;

	if (currrun >= runRF) 
		snprintf(temp, sizeof(temp), "%s",
				get_var(UI_DOC, VAR_FIXED, htTY));
	else 
		snprintf(temp, sizeof(temp), "%s", 
				get_var(UI_DOC, VAR_FIXED, htLY));

	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	ass.DOC = newDate(0, atoi(year), atoi(month), atoi(day));
	ass.DR = (currrun >= runNewDR ?
			atof(get_var(UI_DR, VAR_FIXED, htTY)) :
			atof(get_var(UI_DR, VAR_FIXED, htLY)));
	ass.DR113 =
		(currrun >= runNewDR ?
		 atof(get_var(UI_DR113, VAR_FIXED, htTY)) :
		 atof(get_var(UI_DR113, VAR_FIXED, htLY)));
	ass.agecorr = (currrun >= runNewMortality ?
			atoi(get_var(UI_AGECORR, VAR_FIXED, htTY)) :
			atoi(get_var(UI_AGECORR, VAR_FIXED, htLY)));
	ass.infl = (currrun >= runNewInflation ?
			atof(get_var(UI_INFL, VAR_FIXED, htTY)) :
			atof(get_var(UI_INFL, VAR_FIXED, htLY)));
	ass.ct[IA_SS] =
		(currrun >= runNewSS ?
		 plantTree(strclean(get_var(UI_SS, VAR_INTERPRETER, htTY))) :
		 plantTree(strclean(get_var(UI_SS, VAR_INTERPRETER, htLY))));
	ass.ct[IA_NRA] =
		(currrun >= runNewNRA ?
		 plantTree(strclean(get_var(UI_NRA, VAR_INTERPRETER, htTY))) :
		 plantTree(strclean(get_var(UI_NRA, VAR_INTERPRETER, htLY))));
	ass.ct[IA_WXDEF] =
		(currrun >= runNewTurnover ?
		 plantTree(strclean(get_var(UI_TURNOVER, VAR_INTERPRETER,
					 htTY))) :
		 plantTree(strclean(get_var(UI_TURNOVER, VAR_INTERPRETER,
					 htLY))));
	ass.ct[IA_RETX] =
		(currrun >= runNewNRA ?
		 plantTree(strclean(get_var(UI_RETX, VAR_INTERPRETER,
					 htTY))) :
		 plantTree(strclean(get_var(UI_RETX, VAR_INTERPRETER,
					 htLY))));
	ass.ct[IA_CALCA] = 
		(currrun >= runNewMethodology ?
		 plantTree(strclean(get_var(UI_CONTRA, VAR_INTERPRETER,
					 htTY))) :
		 plantTree(strclean(get_var(UI_CONTRA, VAR_INTERPRETER,
					 htLY))));
	ass.ct[IA_CALCC] = 0;
	ass.ct[IA_CALCDTH] = 0;

	// Assumptions that usually won't change from year to year
	ass.incrSalk0 = 0; // determine whether sal gets increased at k = 0
	ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1
	ass.TRM_PercDef = atof(get_var(UI_TRM_PERCDEF, VAR_FIXED, htTY));

	if (currrun >= runNewMethodology) {
		set_methodology(htTY);
	} else {
		set_methodology(htLY);
	}
}

static const char *get_var(unsigned ui, unsigned var_type,
		Hashtable ht[static 1])
{
	const char *s = 0;
	s = ht_get(get_ui_key(ui, var_type), ht);	
	return s;
}

static void set_methodology(Hashtable ht[static 1])
{
	if (atoi(get_var(COMBO_STANDARD, VAR_COMBO, ht))) ass.method += mIAS;
	if (PAR115 == atoi(get_var(COMBO_ASSETS, VAR_COMBO, ht)))
		ass.method += mPAR115;
	if (RES == atoi(get_var(COMBO_ASSETS, VAR_COMBO, ht)))
		ass.method += mRES;
	if (TUC == atoi(get_var(COMBO_ASSETS, VAR_COMBO, ht)))
		ass.method += mTUC;
	if (atoi(get_var(COMBO_MAXPUCTUC, VAR_COMBO, ht)))
		ass.method += mmaxPUCTUC;
	if (atoi(get_var(COMBO_MAXERCONTR, VAR_COMBO, ht)))
		ass.method += mmaxERContr;
	if (atoi(get_var(COMBO_EVALDTH, VAR_COMBO, ht))) ass.method += mDTH;

	if (ass.method & mIAS)
		ass.taxes = 0.0886 + 0.044;
	else
		ass.taxes = 0;

}

void set_tariffs(const CurrentMember cm[static 1])
{
	unsigned ltins = 0;
	unsigned ltterm = 0;
	struct casetree *ct = 0;
	Hashtable *ht = 0;
	/* this needs updating when we have a UITY!!! */
	if (currrun >= runNewData)
		ht = get_user_input(USER_INPUT_LY);
	else
		ht = get_user_input(USER_INPUT_LY);

	replant(&ct, ht, UI_ADMINCOST);
	tff.admincost = interpret(ct, parameters);
	replant(&ct, ht, UI_COSTRES);
	tff.costRES = interpret(ct, parameters);
	replant(&ct, ht, UI_COSTKO);
	tff.costKO = interpret(ct, parameters);
	replant(&ct, ht, UI_WD);
	tff.WDDTH = interpret(ct, parameters);
	tff.MIXEDPS = (currrun >= runNewData ? 1 : 1);
	replant(&ct, ht, UI_PREPOST);
	tff.prepost = interpret(ct, parameters);
	replant(&ct, ht, UI_TERM);
	tff.term = interpret(ct, parameters);

	replant(&ct, ht, UI_LTINS);
	ltins = interpret(ct, parameters);
	replant(&ct, ht, UI_LTTERM);
	ltterm = interpret(ct, parameters);
	for (int l = 0; l < EREE_AMOUNT; l++) {
		for (int j = 0; j < MAXGEN; j++) {
			tff.ltINS[l][j].lt = ltins;
			tff.ltAfterTRM[l][j].lt = ltterm;
			tff.ltINS[l][j].i = cm->TAUX[l][j];
			tff.ltAfterTRM[l][j].i = cm->TAUX[l][j];
		}
		tff.ltProlong[l].lt = tff.ltINS[l][0].lt;
		tff.ltProlongAfterTRM[l].lt = tff.ltAfterTRM[l][0].lt;
		tff.ltProlong[l].i = tff.ltINS[l][MAXGEN-1].i;
		tff.ltProlongAfterTRM[l].i = tff.ltAfterTRM[l][MAXGEN-1].i;
	}

	chopTree(ct);
}

static void replant(struct casetree **ct, Hashtable *ht, unsigned it)
{
	assert(it < UI_ADMINCOST);
	chopTree(*ct);
	*ct = plantTree(get_var(it, UI_INT, ht));
	return;
}

/*
 * sets the parameters used in the interpreter.
 */
void setparameters(const CurrentMember cm[static 1], int k)
{
	/* this needs updating when we have a UITY!!! */
	Hashtable *ht = get_user_input(USER_INPUT_LY);

	size_t len = MAX_STRING_SIZE;
	double sex = 1.0;
	double lt[LT_AMOUNT] = {
		LXMR, LXFR, LXMK, LXFK, LXFKP, LXNIHIL
	};

	if (cm->status & MALE)
		sex = 1.0;
	else
		sex = 2.0;

	parameters[VAR_AGE].d = cm->age[k];
	snprintf(parameters[VAR_REG].s, len, "%s", cm->regl);
	snprintf(parameters[VAR_CAT].s, len, "%s", cm->category);

	if (cm->status & ACT)
		snprintf(parameters[VAR_STATUS].s, len, "%s", "ACT");
	else
		snprintf(parameters[VAR_STATUS].s, len, "%s", "DEF");
	
	parameters[VAR_SEX].d = sex;
	parameters[VAR_SAL].d = cm->sal[k];
	parameters[VAR_PT].d = cm->PT;
	parameters[VAR_NDOA].d = cm->nDOA[k];
	parameters[VAR_NDOE].d = cm->nDOE[k];

	snprintf(parameters[VAR_COMBINATION].s, len, "%s",
			inscomb[cm->tariff]);

	for (unsigned i = VAR_LXMR; i < LT_AMOUNT + VAR_LXMR; i++)
		parameters[i].d = lt[i - VAR_LXMR];

	init_var(parameters);
}

double salaryscale(const CurrentMember cm[static 1], int k)
{
	return ass.infl + interpret(ass.ct[IA_SS], parameters);
}

double calcA(const CurrentMember cm[static 1], int k)
{
	return interpret(ass.ct[IA_CALCA], parameters);
}

double calcC(const CurrentMember cm[static 1], int k)
{
	return gensum(cm->PREMIUM, EE, 0);
}

double calcDTH(const CurrentMember cm[static 1], int k)
{
	return gensum(cm->CAPDTH, ER, 0);
}

double NRA(const CurrentMember cm[static 1], int k)
{
	return interpret(ass.ct[IA_NRA], parameters);
}

double wxdef(const CurrentMember cm[static 1], int k)
{
	return interpret(ass.ct[IA_WXDEF], parameters);
}

double retx(const CurrentMember cm[static 1], int k)
{
	return interpret(ass.ct[IA_RETX], parameters);
}
