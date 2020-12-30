#include "DCProgram.h"

static double salaryscaleTY(CurrentMember *cm, int k);
static double salaryscaleLY(CurrentMember *cm, int k);
static double NRATY(CurrentMember *cm, int k);
static double NRALY(CurrentMember *cm, int k);
static double wxdefTY(CurrentMember *cm, int k);
static double wxdefLY(CurrentMember *cm, int k);
static double wximmTY(CurrentMember *cm, int k);
static double wximmLY(CurrentMember *cm, int k);
static double calcATY(CurrentMember *cm, int k);
static double calcALY(CurrentMember *cm, int k);
static double calcCTY(CurrentMember *cm, int k);
static double calcCLY(CurrentMember *cm, int k);

static LifeTable ltINS[2][MAXGEN];
static LifeTable ltAfterTRM[2][MAXGEN];
static LifeTable ltProlong[2];
static LifeTable ltProlongAfterTRM[2];

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
  ass.wximm = (currrun >= runNewTurnover ? wximmTY : wximmLY);
  ass.calcA = (currrun >= runNewMethodology ? calcATY : calcALY);
  ass.calcC = (currrun >= runNewMethodology ? calcCTY : calcCLY);    

  // Assumptions that usually won't change from year to year
  ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1

  //---Tariff Structure---
  //-  Life Tables  -
  tff.ltINS = &ltINS;
  tff.ltAfterTRM = &ltAfterTRM;
  tff.ltProlong = &ltProlong;
  tff.ltProlongAfterTRM = &ltProlongAfterTRM;

  for (int EREE = 0; EREE < 2; EREE++) {
    for (int j = 0; j < MAXGEN; j++) {
      switch(cm->tariff) {
      case UKMS :
	tff.ltINS[EREE][j]->lt = lifetables[LXNIHIL];
	tff.ltAfterTRM[EREE][j]->lt = lifetables[LXNIHIL];
	break;
      case UKZT :
	if (cm->status & ACT) {
	  tff.ltINS[EREE][j]->lt = (cm->status & MALE ? lifetables[LXMK] : lifetables[LXFKP]);
	  tff.ltAfterTRM[EREE][j]->lt = (cm->status & MALE ? lifetables[LXMR] : lifetables[LXFR]);
	}
	else {
	  tff.ltINS[EREE][j]->lt = (cm->status & MALE ? lifetables[LXMR] : lifetables[LXFR]);
	  tff.ltAfterTRM[EREE][j]->lt = tff.ltINS[EREE][j]->lt;
	}
	break;
      case UKMT :
	tff.ltINS[EREE][j]->lt = (cm->status & MALE ? lifetables[LXMK] : lifetables[LXFKP]);
	tff.ltAfterTRM[EREE][j]->lt = tff.ltINS[EREE][j]->lt;
	break;
      case MIXED :
	tff.ltINS[EREE][j]->lt = (cm->status & MALE ? lifetables[LXMK] : lifetables[LXFKP]);
	tff.ltAfterTRM[EREE][j]->lt = tff.ltINS[EREE][j]->lt;
	break;
      }
      tff.ltINS[EREE][j]->i = cm->TAUX[EREE][j];
      tff.ltAfterTRM[EREE][j]->i = cm->TAUX[EREE][j];
    }
    tff.ltProlong[EREE]->lt = tff.ltINS[EREE][0]->lt; // Any generation would be ok, I chose 0.
    tff.ltProlongAfterTRM[EREE]->lt = tff.ltAfterTRM[EREE][0]->lt;
    tff.ltProlong[EREE]->i = tff.ltINS[EREE][MAXGEN-1]->i;
    tff.ltProlongAfterTRM[EREE]->i = tff.ltAfterTRM[EREE][MAXGEN-1]->i;    
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

static double wximmTY(CurrentMember *cm, int k) {
  // Percentage of people who take their reserves with them at turnover is assumed to be 0
  return 0;
}

static double wximmLY(CurrentMember *cm, int k) {
  // Percentage of people who take their reserves with them at turnover is assumed to be 0
  return 0;
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
