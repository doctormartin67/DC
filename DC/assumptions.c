#include <math.h>
#include "assumptions.h"
#include "lifetables.h"

static union value parameters[VAR_AMOUNT];

/*
 * This array holds extra variables for example ceilings.
 * The variables are increased with inflation
 */
double extra_var[EXTRA_AMOUNT][MAXPROJ];

static void set_methodology(struct user_input ui[static 1]);
static void replant(struct casetree **ct, const struct user_input ui[static 1],
		unsigned it);

void setassumptions(void)
{
	/* this needs updating when we have a UITY!!! */
	struct user_input *UITY = get_user_input(USER_INPUT_LY);
	struct user_input *UILY = get_user_input(USER_INPUT_LY);
	char temp[BUFSIZ];
	char *year, *month, *day;
	year = month = day = 0;

	if (currrun >= runRF) 
		snprintf(temp, sizeof(temp), "%s", UITY->var[UI_DOC]);
	else 
		snprintf(temp, sizeof(temp), "%s", UILY->var[UI_DOC]);

	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	ass.DOC = newDate(0, atoi(year), atoi(month), atoi(day));
	ass.DR = (currrun >= runNewDR ?
			atof(UITY->var[UI_DR]) :
			atof(UILY->var[UI_DR]));
	ass.DR113 =
		(currrun >= runNewDR ?
		 atof(UITY->var[UI_DR113]) :
		 atof(UILY->var[UI_DR113]));
	ass.agecorr = (currrun >= runNewMortality ?
			atoi(UITY->var[UI_AGECORR]) :
			atoi(UILY->var[UI_AGECORR]));
	ass.infl = (currrun >= runNewInflation ?
			atof(UITY->var[UI_INFL]) :
			atof(UILY->var[UI_INFL]));
	ass.ct[IA_SS] =
		(currrun >= runNewSS ?
		 plantTree(strclean(UITY->var[UI_SS])) :
		 plantTree(strclean(UILY->var[UI_SS])));
	ass.ct[IA_NRA] =
		(currrun >= runNewNRA ?
		 plantTree(strclean(UITY->var[UI_NRA])) :
		 plantTree(strclean(UILY->var[UI_NRA])));
	ass.ct[IA_WXDEF] =
		(currrun >= runNewTurnover ?
		 plantTree(strclean(UITY->var[UI_TURNOVER])) :
		 plantTree(strclean(UILY->var[UI_TURNOVER])));
	ass.ct[IA_RETX] =
		(currrun >= runNewNRA ?
		 plantTree(strclean(UITY->var[UI_RETX])) :
		 plantTree(strclean(UILY->var[UI_RETX])));
	ass.ct[IA_CALCA] = 
		(currrun >= runNewMethodology ?
		 plantTree(strclean(UITY->var[UI_CONTRA])) :
		 plantTree(strclean(UILY->var[UI_CONTRA])));
	ass.ct[IA_CALCC] = 0;
	ass.ct[IA_CALCDTH] = 0;

	// Assumptions that usually won't change from year to year
	ass.incrSalk0 = 0; // determine whether sal gets increased at k = 0
	ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1
	ass.TRM_PercDef = atof(UITY->var[UI_TRM_PERCDEF]);

	if (currrun >= runNewMethodology) {
		set_methodology(UITY);
	} else {
		set_methodology(UILY);
	}
}

static void set_methodology(struct user_input ui[static 1])
{
	if (ui->method[METH_STANDARD]) ass.method += mIAS;
	if (PAR115 == ui->method[METH_ASSETS]) ass.method += mPAR115;
	if (RES == ui->method[METH_ASSETS]) ass.method += mRES;
	if (TUC == ui->method[METH_DBO]) ass.method += mTUC;
	if (ui->method[METH_MAXPUCTUC]) ass.method += mmaxPUCTUC;
	if (ui->method[METH_MAXERCONTR]) ass.method += mmaxERContr;
	if (ui->method[METH_EVALDTH]) ass.method += mDTH;

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
	struct user_input *ui = 0;
	/* this needs updating when we have a UITY!!! */
	if (currrun >= runNewData)
		ui = get_user_input(USER_INPUT_LY);
	else
		ui = get_user_input(USER_INPUT_LY);

	replant(&ct, ui, UI_ADMINCOST);
	tff.admincost = interpret(ct, parameters);
	replant(&ct, ui, UI_COSTRES);
	tff.costRES = interpret(ct, parameters);
	replant(&ct, ui, UI_COSTKO);
	tff.costKO = interpret(ct, parameters);
	replant(&ct, ui, UI_WD);
	tff.WDDTH = interpret(ct, parameters);
	tff.MIXEDPS = (currrun >= runNewData ? 1 : 1);
	replant(&ct, ui, UI_PREPOST);
	tff.prepost = interpret(ct, parameters);
	replant(&ct, ui, UI_TERM);
	tff.term = interpret(ct, parameters);

	replant(&ct, ui, UI_LTINS);
	ltins = interpret(ct, parameters);
	replant(&ct, ui, UI_LTTERM);
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

static void replant(struct casetree **ct, const struct user_input *ui,
		unsigned it)
{
	if (it >= UI_AMOUNT) die("Unknown casetree");
	chopTree(*ct);
	*ct = plantTree(strclean(ui->var[it]));
}

/*
 * sets the parameters used in the interpreter.
 */
void setparameters(const CurrentMember cm[static 1], int k)
{
	/* this needs updating when we have a UITY!!! */
	struct user_input *UITY = get_user_input(USER_INPUT_LY);

	size_t len = MAX_STRING_SIZE;
	double sex = 1.0;
	double lt[LT_AMOUNT] = {
		LXMR, LXFR, LXMK, LXFK, LXFKP, LXNIHIL
	};

	for (unsigned i = 0; i < EXTRA_AMOUNT; i++) {
		if (0 == k)
			extra_var[i][k] = atof(UITY->var[i + UI_EXTRA]);
		if (0 < k)	
			extra_var[i][k] = extra_var[i][k-1]
				* pow((1 + ass.infl),
						cm->DOC[k]->year 
						- cm->DOC[k-1]->year 
						+ (1 == k ? ass.incrSalk1 :0));
	}

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

	/* this needs updating when we have a UITY!!! */
	for (unsigned i = VAR_CEIL1; i < EXTRA_AMOUNT + VAR_CEIL1; i++)
		parameters[i].d = extra_var[i - VAR_CEIL1][k];
	
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
