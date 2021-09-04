#include "DCProgram.h"
#include "lifetables.h"

static double salaryscaleTY(const CurrentMember cm[static 1], int k);
static double salaryscaleLY(const CurrentMember cm[static 1], int k);
static double calcATY(const CurrentMember cm[static 1], int k);
static double calcALY(const CurrentMember cm[static 1], int k);
static double calcCTY(const CurrentMember cm[static 1], int k);
static double calcCLY(const CurrentMember cm[static 1], int k);
static double calcDTHTY(const CurrentMember cm[static 1], int k);
static double calcDTHLY(const CurrentMember cm[static 1], int k);
static double NRATY(const CurrentMember cm[static 1], int k);
static double NRALY(const CurrentMember cm[static 1], int k);
static double wxdefTY(const CurrentMember cm[static 1], int k);
static double wxdefLY(const CurrentMember cm[static 1], int k);
static double retxTY(const CurrentMember cm[static 1], int k);
static double retxLY(const CurrentMember cm[static 1], int k);

void setassumptions(CurrentMember cm[static 1],
		UserInput UILY[static 1], UserInput UITY[static 1])
{
    char temp[BUFSIZ];
    char *year, *month, *day;

    year = month = day = 0;

    if (currrun >= runRF) 
	    snprintf(temp, sizeof(temp), "%s", UITY->DOC);
    else 
	    snprintf(temp, sizeof(temp), "%s", UILY->DOC);

    day = strtok(temp, "/");
    month = strtok(NULL, "/");
    year = strtok(NULL, "");
    
    ass.DOC = newDate(0, atoi(year), atoi(month), atoi(day));
    cm->DOC[1] = ass.DOC;
    ass.DR = (currrun >= runNewDR ? atof(UITY->DR) : atof(UILY->DR));
    ass.DR113 = (currrun >= runNewDR ? atof(UITY->DR113) : atof(UILY->DR113));
    ass.agecorr = (currrun >= runNewMortality
		    ? atoi(UITY->agecorr) : atoi(UILY->agecorr));
    ass.SS = (currrun >= runNewSS ? salaryscaleTY : salaryscaleLY);
    ass.infl = (currrun >= runNewInflation
		    ? atof(UITY->infl) : atof(UILY->infl));
    ass.NRA = (currrun >= runNewNRA ? NRATY : NRALY);
    ass.wxdef = (currrun >= runNewTurnover ? wxdefTY : wxdefLY);
    ass.retx = (currrun >= runNewNRA ? retxTY : retxLY);
    ass.calcA = (currrun >= runNewMethodology ? calcATY : calcALY);
    ass.calcC = (currrun >= runNewMethodology ? calcCTY : calcCLY);    
    ass.calcDTH = (currrun >= runNewMethodology ? calcDTHTY : calcDTHLY);    

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

    //---Tariff Structure---
    //-  Life Tables  -

    for (int l = 0; l < EREE_AMOUNT; l++) {
	for (int j = 0; j < MAXGEN; j++) {
	    switch(cm->tariff) {
		case UKMS :
		    tff.ltINS[l][j].lt = LXNIHIL;
		    tff.ltAfterTRM[l][j].lt = LXNIHIL;
		    break;
		case UKZT :
		    if (cm->status & ACT) {
			tff.ltINS[l][j].lt = (cm->status & MALE ? LXMK : LXFKP);
			tff.ltAfterTRM[l][j].lt = (cm->status & MALE ? LXMR : LXFR);
		    } else {
			tff.ltINS[l][j].lt = (cm->status & MALE ? LXMR : LXFR);
			tff.ltAfterTRM[l][j].lt = tff.ltINS[l][j].lt;
		    }
		    break;
		case UKMT :
		    tff.ltINS[l][j].lt = (cm->status & MALE ? LXMK : LXFKP);
		    tff.ltAfterTRM[l][j].lt = tff.ltINS[l][j].lt;
		    break;
		case MIXED :
		    tff.ltINS[l][j].lt = (cm->status & MALE ? LXMK : LXFKP);
		    tff.ltAfterTRM[l][j].lt = tff.ltINS[l][j].lt;
		    break;
	    }
	    tff.ltINS[l][j].i = cm->TAUX[l][j];
	    tff.ltAfterTRM[l][j].i = cm->TAUX[l][j];
	}
	tff.ltProlong[l].lt = tff.ltINS[l][0].lt; // Any generation would be ok, I chose 0.
	tff.ltProlongAfterTRM[l].lt = tff.ltAfterTRM[l][0].lt;
	tff.ltProlong[l].i = tff.ltINS[l][MAXGEN-1].i;
	tff.ltProlongAfterTRM[l].i = tff.ltAfterTRM[l][MAXGEN-1].i;    
    }
    //-  Remaining Tariffs  -
    tff.costRES = (currrun >= runNewData ? 0.001 : 0.001);
    tff.WDDTH = (currrun >= runNewData ? 0.3 : 0.3);
    tff.costKO = (currrun >= runNewData ? 0.0008 : 0.0008);
    tff.admincost = (currrun >= runNewData ? 0.05 : 0.05);
    tff.MIXEDPS = (currrun >= runNewData ? 1 : 1);
    tff.prepost = (currrun >= runNewData ? 1 : 1);
    tff.term = (currrun >= runNewData ? 12 : 12);
}

static double salaryscaleTY(const CurrentMember cm[static 1], int k)
{
    return ass.infl + 0.011;
}

static double salaryscaleLY(const CurrentMember cm[static 1], int k)
{
    return ass.infl + 0.011;
}

static double calcATY(const CurrentMember cm[static 1], int k)
{
    // This needs updating! just took insurer premium for testing
    return gensum(cm->PREMIUM, ER, 0);
}

static double calcALY(const CurrentMember cm[static 1], int k)
{
    // This needs updating! just took insurer premium for testing
    return gensum(cm->PREMIUM, ER, 0);
}

static double calcCTY(const CurrentMember cm[static 1], int k)
{
    // This needs updating! just took insurer premium for testing
    return gensum(cm->PREMIUM, EE, 0);
}
static double calcCLY(const CurrentMember cm[static 1], int k)
{
    // This needs updating! just took insurer premium for testing
    return gensum(cm->PREMIUM, EE, 0);
}

static double calcDTHTY(const CurrentMember cm[static 1], int k)
{
    // This needs updating! just took insurer premium for testing
    return gensum(cm->CAPDTH, EE, 0);
}

static double calcDTHLY(const CurrentMember cm[static 1], int k)
{
    // This needs updating! just took insurer premium for testing
    return gensum(cm->CAPDTH, EE, 0);
}

static double NRATY(const CurrentMember cm[static 1], int k)
{
    return 65;
}

static double NRALY(const CurrentMember cm[static 1], int k)
{
    return 65;
}

static double wxdefTY(const CurrentMember cm[static 1], int k)
{
    if (cm->age[k] < 60)
	return 0.01;
    else
	return 0;
}

static double wxdefLY(const CurrentMember cm[static 1], int k)
{
    if (cm->age[k] < 60)
	return 0.01;
    else
	return 0;
}

static double retxTY(const CurrentMember cm[static 1], int k)
{
    if (cm->age[k] < 65)
	return 0;
    else
	return 1;
}

static double retxLY(const CurrentMember cm[static 1], int k)
{
    if (cm->age[k] < 65)
	return 0;
    else
	return 1;
}
