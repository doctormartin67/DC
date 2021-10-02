#include "assumptions.h"
#include "lifetables.h"

static const void *parameters[VAR_AMOUNT];

static void set_methodology(struct user_input ui[static 1]);
static void setparameters(const CurrentMember cm[static 1], int k);

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
	ass.ct[IA_CALCA] = 0;
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

static void setparameters(const CurrentMember cm[static 1], int k)
{
	parameters[AGE] = &cm->age[k];
	parameters[REG] = cm->regl;
	parameters[CAT] = cm->category;
}

double salaryscale(const CurrentMember cm[static 1], int k)
{
	setparameters(cm, k);
	return ass.infl + interpret(ass.ct[IA_SS], parameters);
}

double calcA(const CurrentMember cm[static 1], int k)
{
	return gensum(cm->PREMIUM, ER, 0);
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
	setparameters(cm, k);
	return interpret(ass.ct[IA_NRA], parameters);
}

double wxdef(const CurrentMember cm[static 1], int k)
{
	setparameters(cm, k);
	return interpret(ass.ct[IA_WXDEF], parameters);
}

double retx(const CurrentMember cm[static 1], int k)
{
	setparameters(cm, k);
	return interpret(ass.ct[IA_RETX], parameters);
}
