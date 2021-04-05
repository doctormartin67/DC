#include "DCProgram.h"
#include "lifetables.h"

static double salaryscaleTY(CurrentMember *cm, int k);
static double salaryscaleLY(CurrentMember *cm, int k);
static double calcATY(CurrentMember *cm, int k);
static double calcALY(CurrentMember *cm, int k);
static double calcCTY(CurrentMember *cm, int k);
static double calcCLY(CurrentMember *cm, int k);
static double calcDTHTY(CurrentMember *cm, int k);
static double calcDTHLY(CurrentMember *cm, int k);
static double NRATY(CurrentMember *cm, int k);
static double NRALY(CurrentMember *cm, int k);
static double wxdefTY(CurrentMember *cm, int k);
static double wxdefLY(CurrentMember *cm, int k);
static double retxTY(CurrentMember *cm, int k);
static double retxLY(CurrentMember *cm, int k);

void setassumptions(CurrentMember *cm) {
	ass.DOC = (currrun >= runRF ? newDate(0, 2016, 12, 1) : newDate(0, 2015, 12, 1));
	// DOC[0] = DOS so we start here with defining DOC[1]
	cm->DOC[1] = ass.DOC;
	ass.DR = (currrun >= runNewDR ? 0.0125 : 0.0125);
	ass.DR113 = (currrun >= runNewDR ? 0.0185 : 0.0185);
	ass.agecorr = (currrun >= runNewMortality ? -3 : -3);
	ass.SS = (currrun >= runNewSS ? salaryscaleTY : salaryscaleLY);
	ass.infl = (currrun >= runNewInflation ? 0.018 : 0.018);
	ass.NRA = (currrun >= runNewNRA ? NRATY : NRALY);
	ass.wxdef = (currrun >= runNewTurnover ? wxdefTY : wxdefLY);
	ass.retx = (currrun >= runNewNRA ? retxTY : retxLY);
	ass.calcA = (currrun >= runNewMethodology ? calcATY : calcALY);
	ass.calcC = (currrun >= runNewMethodology ? calcCTY : calcCLY);    
	ass.calcDTH = (currrun >= runNewMethodology ? calcDTHTY : calcDTHLY);    

	// Assumptions that usually won't change from year to year
	ass.incrSalk0 = 0; // determine whether sal gets increased at k = 0
	ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1
	ass.TRM_PercDef = 1;
	ass.method = mPAR115;
	// mIAS, mTUC, mmaxERContr and mmaxPUCTUC still need to be added here
	if (ass.method & mIAS)
		ass.taxes = 0.1326; // 0.0886 + 0.044
	else
		ass.taxes = 0; // in case of FAS calculation, no taxes
	ass.method += mDTH; // death is evaluated

	//---Tariff Structure---
	//-  Life Tables  -

	for (int EREE = 0; EREE < 2; EREE++) {
		for (int j = 0; j < MAXGEN; j++) {
			switch(cm->tariff) {
				case UKMS :
					tff.ltINS[EREE][j].lt = LXNIHIL;
					tff.ltAfterTRM[EREE][j].lt = LXNIHIL;
					break;
				case UKZT :
					if (cm->status & ACT) {
						tff.ltINS[EREE][j].lt = (cm->status & MALE ? LXMK : LXFKP);
						tff.ltAfterTRM[EREE][j].lt = (cm->status & MALE ? LXMR : LXFR);
					}
					else {
						tff.ltINS[EREE][j].lt = (cm->status & MALE ? LXMR : LXFR);
						tff.ltAfterTRM[EREE][j].lt = tff.ltINS[EREE][j].lt;
					}
					break;
				case UKMT :
					tff.ltINS[EREE][j].lt = (cm->status & MALE ? LXMK : LXFKP);
					tff.ltAfterTRM[EREE][j].lt = tff.ltINS[EREE][j].lt;
					break;
				case MIXED :
					tff.ltINS[EREE][j].lt = (cm->status & MALE ? LXMK : LXFKP);
					tff.ltAfterTRM[EREE][j].lt = tff.ltINS[EREE][j].lt;
					break;
			}
			tff.ltINS[EREE][j].i = cm->TAUX[EREE][j];
			tff.ltAfterTRM[EREE][j].i = cm->TAUX[EREE][j];
		}
		tff.ltProlong[EREE].lt = tff.ltINS[EREE][0].lt; // Any generation would be ok, I chose 0.
		tff.ltProlongAfterTRM[EREE].lt = tff.ltAfterTRM[EREE][0].lt;
		tff.ltProlong[EREE].i = tff.ltINS[EREE][MAXGEN-1].i;
		tff.ltProlongAfterTRM[EREE].i = tff.ltAfterTRM[EREE][MAXGEN-1].i;    
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

static double salaryscaleTY(CurrentMember *cm, int k) {
	return ass.infl + 0.011;
}

static double salaryscaleLY(CurrentMember *cm, int k) {
	return ass.infl + 0.011;
}

static double calcATY(CurrentMember *cm, int k) {
	// This needs updating! just took insurer premium for testing
	return gensum(cm->PREMIUM, ER, 0);
}

static double calcALY(CurrentMember *cm, int k) {
	// This needs updating! just took insurer premium for testing
	return gensum(cm->PREMIUM, ER, 0);
}

static double calcCTY(CurrentMember *cm, int k) {
	// This needs updating! just took insurer premium for testing
	return gensum(cm->PREMIUM, EE, 0);
}
static double calcCLY(CurrentMember *cm, int k) {
	// This needs updating! just took insurer premium for testing
	return gensum(cm->PREMIUM, EE, 0);
}

static double calcDTHTY(CurrentMember *cm, int k) {
	// This needs updating! just took insurer premium for testing
	return gensum(cm->CAPDTH, EE, 0);
}
static double calcDTHLY(CurrentMember *cm, int k) {
	// This needs updating! just took insurer premium for testing
	return gensum(cm->CAPDTH, EE, 0);
}

static double NRATY(CurrentMember *cm, int k) {
	return 65;
}

static double NRALY(CurrentMember *cm, int k) {
	return 65;
}

static double wxdefTY(CurrentMember *cm, int k) {
	if (cm->age[k] < 60)
		return 0.01;
	else
		return 0;
}

static double wxdefLY(CurrentMember *cm, int k) {
	if (cm->age[k] < 60)
		return 0.01;
	else
		return 0;
}

static double retxTY(CurrentMember *cm, int k) {
	if (cm->age[k] < 65)
		return 0;
	else
		return 1;
}

static double retxLY(CurrentMember *cm, int k) {
	if (cm->age[k] < 65)
		return 0;
	else
		return 1;
}

