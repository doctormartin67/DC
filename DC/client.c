#include "DCProgram.h"

Assumptions assLY;
Assumptions assTY;

void setassumptions(CurrentMember *cm) {
  assTY.DOC = newDate(0, 2016, 12, 1);
  *cm->DOC = assTY.DOC;
  assTY.DR = 0.0125;
  assTY.DR113 = 0.0185;
  assTY.agecorr = -3;
  assTY.SS = salaryscale;
  assTY.infl = 0.018;
  assTY.NRA = NRA;
  assTY.wxdef = wxdef;
  assTY.wximm = wximm;
}

double salaryscale(CurrentMember *cm) {
  return assTY.infl + 0.011;
}

unsigned short NRA(CurrentMember *cm) {
  return 65;
}

double wxdef(CurrentMember *cm) {
  if (*cm->age < 60)
    return 0.01;
  else
    return 0;
}

double wximm(CurrentMember *cm) {
  // Percentage of people who take their reserves with them at turnover is assumed to be 0
  return 0;
}

