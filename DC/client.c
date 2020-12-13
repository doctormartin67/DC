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

  //Assumptions that usually won't change from year to year
  ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1

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

double salaryscale(CurrentMember *cm, int k) {
  return (*ass.SS)(cm, k);
}

unsigned short NRA(CurrentMember *cm, int k) {
  return (*ass.NRA)(cm, k);
}

double wxdef(CurrentMember *cm, int k) {
  return (*ass.wxdef)(cm, k);
}

double wximm(CurrentMember *cm, int k) {
  return (*ass.wximm)(cm, k);
}


