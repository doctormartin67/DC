#include "assumptions.h"
#include "lifetables.h"

static const void *parameters[VAR_AMOUNT];

static void setparameters(const CurrentMember cm[static 1], int k);

void setassumptions(struct user_input UILY[static 1],
		struct user_input UITY[static 1])
{
	char temp[BUFSIZ];
	char *year, *month, *day;

	year = month = day = 0;

	if (currrun >= runRF) 
		snprintf(temp, sizeof(temp), "%s", UITY->DOC);
	else 
		snprintf(temp, sizeof(temp), "%s", UILY->DOC);

	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	ass.DOC = newDate(0, atoi(year), atoi(month), atoi(day));
	ass.DR = (currrun >= runNewDR ? atof(UITY->DR) : atof(UILY->DR));
	ass.DR113 =
		(currrun >= runNewDR ? atof(UITY->DR113) : atof(UILY->DR113));
	ass.agecorr = (currrun >= runNewMortality ?
			atoi(UITY->agecorr) : atoi(UILY->agecorr));
	ass.infl = (currrun >= runNewInflation ?
			atof(UITY->infl) : atof(UILY->infl));
	ass.ct[IA_SS] =
		(currrun >= runNewSS ?
		 plantTree(strclean(UITY->SS)) :
		 plantTree(strclean(UILY->SS)));
	ass.ct[IA_NRA] =
		(currrun >= runNewNRA ?
		 plantTree(strclean(UITY->NRA)) :
		 plantTree(strclean(UILY->NRA)));
	ass.ct[IA_WXDEF] =
		(currrun >= runNewTurnover ?
		 plantTree(strclean(UITY->turnover)) :
		 plantTree(strclean(UILY->turnover)));
	ass.ct[IA_RETX] =
		(currrun >= runNewNRA ?
		 plantTree(strclean(UITY->retx)) :
		 plantTree(strclean(UILY->retx)));
	ass.ct[IA_CALCA] = 0;
	ass.ct[IA_CALCC] = 0;
	ass.ct[IA_CALCDTH] = 0;

	// Assumptions that usually won't change from year to year
	ass.incrSalk0 = 0; // determine whether sal gets increased at k = 0
	ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1
	ass.TRM_PercDef = atof(UITY->TRM_PercDef);
	ass.method = mPAR115;
	// mIAS, mTUC, mmaxERContr and mmaxPUCTUC still need to be added here
	if (ass.method & mIAS)
		ass.taxes = 0.1326; // 0.0886 + 0.044
	else
		ass.taxes = 0; // in case of FAS calculation, no taxes
	ass.method += mDTH; // death is evaluated
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
