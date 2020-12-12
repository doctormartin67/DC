#include "DCProgram.h"

Assumptions assLY;
Assumptions assTY;

static double salaryscaleTY(CurrentMember *cm);
static double salaryscaleLY(CurrentMember *cm);
static unsigned short NRATY(CurrentMember *cm);
static unsigned short NRALY(CurrentMember *cm);
static double wxdefTY(CurrentMember *cm, int k);
static double wxdefLY(CurrentMember *cm, int k);
static double wximmTY(CurrentMember *cm);
static double wximmLY(CurrentMember *cm);

void setassumptions(CurrentMember *cm) {
  assTY.DOC = newDate(0, 2016, 12, 1);
  assLY.DOC = newDate(0, 2015, 12, 1);
  
  // DOC[0] = DOS so we start here with defining DOC[1]
  cm->DOC[1] = assTY.DOC; //needs updating when reconciliation runs get implemented!!

  assTY.DR = 0.0125;
  assLY.DR = 0.0125;

  assTY.DR113 = 0.0185;
  assLY.DR113 = 0.0185;

  assTY.agecorr = -3;
  assLY.agecorr = -3;

  assTY.SS = salaryscaleTY;
  assLY.SS = salaryscaleLY;

  assTY.infl = 0.018;
  assLY.infl = 0.018;

  assTY.NRA = NRATY;
  assLY.NRA = NRALY;

  assTY.wxdef = wxdefTY;
  assLY.wxdef = wxdefLY;

  assTY.wximm = wximmTY;
  assLY.wximm = wximmLY;
}

static double salaryscaleTY(CurrentMember *cm) {
  return assTY.infl + 0.011;
}

static double salaryscaleLY(CurrentMember *cm) {
  return assLY.infl + 0.011;
}

static unsigned short NRATY(CurrentMember *cm) {
  return 65;
}

static unsigned short NRALY(CurrentMember *cm) {
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

static double wximmTY(CurrentMember *cm) {
  // Percentage of people who take their reserves with them at turnover is assumed to be 0
  return 0;
}

static double wximmLY(CurrentMember *cm) {
  // Percentage of people who take their reserves with them at turnover is assumed to be 0
  return 0;
}


