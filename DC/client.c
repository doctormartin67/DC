#include "DCProgram.h"

static double salaryscaleTY(CurrentMember *cm, int k);
static double salaryscaleLY(CurrentMember *cm, int k);
static unsigned short NRATY(CurrentMember *cm, int k);
static unsigned short NRALY(CurrentMember *cm, int k);
static double wxdefTY(CurrentMember *cm, int k);
static double wxdefLY(CurrentMember *cm, int k);
static double wximmTY(CurrentMember *cm, int k);
static double wximmLY(CurrentMember *cm, int k);

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

  // Assumptions that usually won't change from year to year
  ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1

  //---Tariff Structure---
  //-  Life Tables  -

  switch(cm->tariff) {
  case UKMS :
    tff.ltINS = lifetables[LXNIHIL];
    tff.ltAfterTRM = lifetables[LXNIHIL];    
    break;
  case UKZT :
    if (cm->status & ACT) {
      tff.ltINS = (cm->status & MALE ? lifetables[LXMK] : lifetables[LXFKP]);
      tff.ltAfterTRM = (cm->status & MALE ? lifetables[LXMR] : lifetables[LXFR]);
    }
    else {
      tff.ltINS = (cm->status & MALE ? lifetables[LXMR] : lifetables[LXFR]);
      tff.ltAfterTRM = tff.ltINS;
    }
    break;
  case UKMT :
    tff.ltINS = (cm->status & MALE ? lifetables[LXMK] : lifetables[LXFKP]);
    tff.ltAfterTRM = tff.ltINS;
    break;
  case MIXED :
    tff.ltINS = (cm->status & MALE ? lifetables[LXMK] : lifetables[LXFKP]);
    tff.ltAfterTRM = tff.ltINS;
    break;
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

static unsigned short NRATY(CurrentMember *cm, int k) {
  return 65;
}

static unsigned short NRALY(CurrentMember *cm, int k) {
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


