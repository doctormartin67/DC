#include "actuarialfunctions.h"

const double eps = 0.0000001;

double npx(char *lt, double ageX, double ageXn, int corr){ //lt = life table
  /* generally the rule is npx = lx(ageXn)/lx(ageX) but because lx has
     an integer value as its input we need to interpolate both these
     values
  */
  double ip1; //interpolation value 1
  double ip2; //interpolation value 2

  ageX += corr;
  ageXn += corr;
  ageX = fmax(0, ageX);
  ageXn = fmax(0, ageXn);

  ip1 = lx(lt, ageX) - (ageX - (int)ageX) * (lx(lt, ageX) - lx(lt, ageX + 1));
  ip2 = lx(lt, ageXn) - (ageXn - (int)ageXn) * (lx(lt, ageXn) - lx(lt, ageXn + 1));
  
  return ip2/ip1;
}

double nEx(char *lt, double i, double charge, double ageX, double ageXn, int corr){
  double im = (1+i)/(1+charge) - 1;
  double n = ageXn - ageX;
  double vn = 1/pow(1 + im, n);
  double nPx = npx(lt, ageX, ageXn, corr);
  return vn * nPx;
}

double axn(char *lt, double i, double charge, int prepost, int term,
	   double ageX, double ageXn, int corr){
  if (12 % term != 0) {
    printf("An incorrect term was chosen, payments are usually monthly (term = 12)\n");
    printf("but can also be yearly for example (term = 1). term should be divisible\n");
    printf("by 12 but \nterm = %d.\n", term);
    exit(0);
  }
  else if (ageX > ageXn) {
    printf("warning: ageXn = %.6f < ageX = %.6f\n", ageXn, ageX);
    printf("axn = 0 is taken in this case.\n");
    return 0;
  }
  else {
    double ageXk = ageX + (double)prepost/term; // current age in while loop
    int payments = (int)((ageXn - ageX) * term + eps);
    double value = 0; //return value;

    while (payments--) {
      value += nEx(lt, i, charge, ageX, ageXk, corr);
      ageXk += 1.0/term;
    }
    /* There is a final portion that needs to be added when the amount of payments don't consider
       fractions of age, for example if ageX = 40 and ageXn = 40,5 and term = 1. This would give 0 
       payments but we still need to add nEx/2 in this case.
       We also need to subtract one term from ageXk because the while loop adds one too many.
    */    
    ageXk -= 1.0/term * prepost;
    value /= term;
    value += (ageXn - ageXk) *
      nEx(lt, i, charge, ageX,
	  (double)((int)(ageXn*term + eps))/term + term*prepost, corr);
    return value;
  }
}

double Ax1n(char *lt, double i, double charge, double ageX, double ageXn, int corr){
  if (ageX > ageXn) {
    printf("warning: ageXn = %.6f < ageX = %.6f\n", ageXn, ageX);
    printf("Ax1n = 0 is taken in this case.\n");
    return 0;
  }
  else {
    double im = (1+i)/(1+charge) - 1;
    double v = 1/(1 + im);
    int payments = (int)(ageXn - ageX + eps);
    double value = 0; //return value;
    int k = 0;
    // nAx = v^(1/2)*1Qx + v^(1+1/2)*1Px*1q_{x+1} + ... + v^(n-1+1/2)*{n-1}_Px*1Q_{x+n-1}
    while (payments--) {
      value += pow(v, k + 1.0/2) *
	npx(lt, ageX, ageX + k, corr) *
	(1 - npx(lt, ageX + k, ageX + k + 1, corr));
      k++;
    }
    // below is the final fractional payment, for example 40 until 40.6 years old
    value += pow(v, k + 1.0/2) *
      npx(lt, ageX, ageX + k, corr) *
      (1 - npx(lt, ageX + k, ageXn, corr));
    return value;
  }
}

double IAx1n(char *lt, double i, double charge, double ageX, double ageXn, int corr){
  if (ageX > ageXn) {
    printf("warning: ageXn = %.6f < ageX = %.6f\n", ageXn, ageX);
    printf("Ax1n = 0 is taken in this case.\n");
    return 0;
  }
  else {
    int payments = (int)(ageXn - ageX + eps);
    double value = 0; //return value;
    int k = 1;
    // IAx1n = sum^{n-1}_k=1:k*1A_{x+k}*kEx
    while (payments--) {
      value += k * Ax1n(lt, i, charge, ageX + k - 1, ageX + k, corr) *
	nEx(lt, i, charge, ageX, ageX + k - 1, corr);
      k++;
    }
    // below is the final fractional payment, for example 40 until 40.6 years old
    value += k * Ax1n(lt, i, charge, ageX + k - 1, ageXn, corr) *
	nEx(lt, i, charge, ageX, ageXn, corr);
    return value;
  }
}

// This function hasn't been completed because I don't think I need it. At the moment it
// only works for term = 1
double Iaxn(char *lt, double i, double charge, int prepost, int term,
	   double ageX, double ageXn, int corr){
  if (1 % term != 0) {
    printf("Iaxn has only been implemented for term = 1. You will need to adjust your \n");
    printf("input for term, at the moment term = %d.\n", term);
    exit(0);
  }
  else if (ageX > ageXn) {
    printf("warning: ageXn = %.6f < ageX = %.6f\n", ageXn, ageX);
    printf("Iaxn = 0 is taken in this case.\n");
    return 0;
  }
  else {
    double ageXk = ageX + (double)prepost/term; // current age in while loop
    int payments = (int)((ageXn - ageX) * term + eps);
    double value = 0; //return value;
    double k = 1;
    while (payments--) {
      value += k++ * nEx(lt, i, charge, ageX, ageXk, corr);
      ageXk += 1.0/term;
    }
    ageXk -= 1.0/term * prepost;
    value /= term;
    value += (ageXn - ageXk) * k *
      nEx(lt, i, charge, ageX,
	  (double)((int)(ageXn*term + eps))/term + term*prepost, corr);
    return value;
  }
}

void evolCAPDTH(CurrentMember *cm, int EREE, int gen, int k) {
  cm->CAPDTH[EREE][gen][k+1] = cm->CAPDTH[EREE][gen][k] +
    cm->PREMIUM[EREE][gen][k] * (1 - tff.admincost) * (cm->age[k+1] - cm->age[k]);
}

void evolRES(CurrentMember *cm, int EREE, int gen, int k) {
  double cap;
  double RA;
  double i;
  double Ex;

  //---PUC RESERVES---
  RA = (k > MAXPROJBEFOREPROL ? NRA(cm, k) : cm->NRA);

  cap = calcCAP(cm, EREE, gen, k,
		cm->RES[PUC][EREE][gen][k],
		cm->PREMIUM[EREE][gen][k],
		cm->DELTACAP[EREE][k],
		cm->CAPDTH[EREE][gen][k], cm->age[k], RA, tff.ltINS[EREE][gen]);
  cm->RES[PUC][EREE][gen][k+1] = calcRES(cm, EREE, gen, k,
					 cap,
					 cm->PREMIUM[EREE][gen][k],
					 cm->DELTACAP[EREE][k],
					 cm->CAPDTH[EREE][gen][k+1],
					 cm->age[k+1], tff.ltINS[EREE][gen]);
  //-  Capital life  -
  cm->CAP[EREE][gen][k] = cap;

  //-  Reduced Capital  -
  cap = calcCAP(cm, EREE, gen, k+1,
		cm->RES[PUC][EREE][gen][k+1],
		0,
		cm->DELTACAP[EREE][k+1],
		cm->CAPDTH[EREE][gen][k+1], cm->age[k+1], RA, tff.ltAfterTRM[EREE][gen]);
  cm->REDCAP[PUC][EREE][gen][k+1] = calcCAP(cm, EREE, gen, k+1,
					    cap,
					    0, 0,
					    cm->CAPDTH[EREE][gen][k+1],
					    RA, NRA(cm, k), tff.ltAfterTRM[EREE][gen]);

  
  //-  RESERVES PROFIT SHARING  -
  cap = calcCAP(cm, EREE, gen, k,
		cm->RESPS[PUC][EREE][gen][k],
		0, 0, 0,
		cm->age[k], RA, tff.ltINS[EREE][gen]);
  cm->RESPS[PUC][EREE][gen][k+1] = calcRES(cm, EREE, gen, k,
					   cap,
					   0, 0, 0,
					   cm->age[k+1], tff.ltINS[EREE][gen]);
  cm->CAPPS[EREE][gen][k] = cap;
  
  //---TUC RESERVES---
  RA = min(3, NRA(cm, k), cm->NRA, cm->age[k+1]);
  i = cm->TAUX[EREE][gen];
  Ex = nEx(tff.ltINS[EREE][gen]->lt, i, tff.costRES, RA, min(2, NRA(cm, k), cm->NRA), 0);

  cap = calcCAP(cm, EREE, gen, k,
		cm->RES[PUC][EREE][gen][1],
		0, 0,
		cm->CAPDTH[EREE][gen][1], cm->age[1], RA, tff.ltINS[EREE][gen]) +
    cm->DELTACAP[EREE][0] * (RA - cm->age[1]) * 12 * Ex;
  cm->RES[TUC][EREE][gen][k+1] = calcCAP(cm, EREE, gen, k,
					 cap,
					 0, 0,
					 cm->CAPDTH[EREE][gen][1],
					 cm->age[k+1], RA, tff.ltAfterTRM[EREE][gen]);

  //-  RESERVES PROFIT SHARING  -
  cm->RESPS[TUC][EREE][gen][k+1] = cm->RESPS[PUC][EREE][gen][k+1];
    
  //---TUCPS_1 RESERVES---
  cap = calcCAP(cm, EREE, gen, k,
		cm->RES[PUC][EREE][gen][(int)min(2, (double)k, 2.0)],
		0, 0,
		cm->CAPDTH[EREE][gen][(int)min(2, (double)k, 2.0)],
		cm->age[(int)min(2, (double)k, 2.0)], RA, tff.ltINS[EREE][gen]) +
    cm->DELTACAP[EREE][0] * (RA - cm->age[(int)min(2, (double)k, 2.0)]) * 12 * Ex;
  cm->RES[TUCPS_1][EREE][gen][k+1] = calcCAP(cm, EREE, gen, k,
					     cap,
					     0, 0,
					     cm->CAPDTH[EREE][gen][(int)min(2, (double)k, 2.0)],
					     cm->age[k+1], RA, tff.ltAfterTRM[EREE][gen]);

  //-  RESERVES PROFIT SHARING  -
  cm->RESPS[TUCPS_1][EREE][gen][k+1] = cm->RESPS[PUC][EREE][gen][k+1];
}

void evolART24(CurrentMember *cm, int k) {

  double i;
  double im;
  double admincost;
  double premium;
  double value; // value to return

  // active member
  if (cm->status & ACT) {
    for (int l = 0; l < 2; l++) { // Employer-Employee
      for (int m = 0; m < ART24GEN2 + 1; m++) { // generation
	i = ART24TAUX[l][m];
	im = pow(1 + i, 1.0/tff.term) - 1; // term interest rate
	admincost = (l == ER ? min(2, ART24admincost, tff.admincost) : 0);
	/* maximum of insurance admin cost and
	   maximum admin cost according to ART24. (Currently 0.05)*/

	for (int j = 0; j < TUCPS_1 + 1; j++) {
	  switch(j) {
	  case PUC :
	    premium = gensum(cm->PREMIUM, l, k);
	    break;
	  case TUC :
	    premium = (k > 0 ? 0 : gensum(cm->PREMIUM, l, k));
	    break;
	  case TUCPS_1 :
	    premium = (k > 1 ? 0 : gensum(cm->PREMIUM, l, k));
	    break;
	  }
	  cm->ART24[j][l][m][k+1] =
	    cm->ART24[j][l][m][k] * pow(1 + i, cm->age[k+1] - cm->age[k]) +
	    (m == ART24GEN1 ? 0 :
	     max(2, 0.0,
		 premium * (1 - admincost) -
		 gensum(cm->RP, l, k)) / tff.term *
	     (pow(1 + im, tff.term * (cm->age[k+1] - cm->age[k]) + (tff.prepost == 0 ? 1 : 0)) -
	      1 - im * (tff.prepost == 0 ? 1 : 0)) / im);
	}
      }
    }
  }
  // deferred member
    else {
      for (int j = 0; j < TUCPS_1 + 1; j++) // Method
	for (int l = 0; l < 2; l++) // Employer-Employee
	  for (int m = 0; m < ART24GEN2 + 1; m++) // generation
	    // ART24 doesn't increase for deferred member
	    cm->ART24[j][l][m][k+1] = cm->ART24[j][l][m][k]; 
    }
  }

double calcCAP(CurrentMember *cm, int EREE, int gen, int k,
	       double res, double prem, double deltacap, double capdth,
	       double age, double RA, LifeTable *lt) {

  double i;
  double Ex;
  double ax;
  
  // These are used for UKMT and MIXED
  double axcost; 
  double Ax1;

  // These are used for UKMT
  double IAx1;
  double Iax;
  
  double value;

  i = lt->i;
  Ex = nEx(lt->lt, i, tff.costRES, age, RA, 0);
  ax = axn(lt->lt, i, tff.costRES, tff.prepost, tff.term, age, RA, 0);
  axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
  Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
  IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
  Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
  
  switch(cm->tariff) {
  case UKMS :
    value = (res + prem * (1 - tff.admincost) * ax) / Ex +
      deltacap * (RA - age) * 12;
    break;
    // same as UKMS
  case UKZT :
    value = (res + prem * (1 - tff.admincost) * ax) / Ex +
      deltacap * (RA - age) * 12;
    break;
  case UKMT :
    value = (res + prem * (1 - tff.admincost) * ax -
	     capdth * (Ax1 + tff.costKO * axcost) -
	     prem * (1 - tff.admincost) * (IAx1 + tff.costKO * Iax)) / Ex;
    break;
  case MIXED :
    value = (res + prem * (1 - tff.admincost) * ax) /
      (Ex + 1.0/cm->X10 * tff.MIXEDPS * (Ax1 + tff.costKO * axcost));
    break;
  }
  
  return value;
}

double calcRES(CurrentMember *cm, int EREE, int gen, int k,
	       double cap, double prem, double deltacap, double capdth,
	       double age, LifeTable *lt) {
  
  unsigned short RA;
  double i;
  double Ex;
  double ax;
  
  // These are used for UKMT and MIXED
  double axcost; 
  double Ax1;

  // These are used for UKMT
  double IAx1;
  double Iax;
  
  double value;

  RA = (k > MAXPROJBEFOREPROL ? NRA(cm, k) : cm->NRA);
  i = lt->i;
  Ex = nEx(lt->lt, i, tff.costRES, age, RA, 0);
  ax = axn(lt->lt, i, tff.costRES, tff.prepost, tff.term, age, RA, 0);
  axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
  Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
  IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
  Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);

  switch(cm->tariff) {
  case UKMS :
    value = (cap - deltacap * (RA - age) * 12) * Ex -
      prem * (1 - tff.admincost) * ax;
    break;
    // same as UKMS
  case UKZT :
    value = (cap - deltacap * (RA - age) * 12) * Ex -
      prem * (1 - tff.admincost) * ax;
    break;
  case UKMT :
    value = cap * Ex - prem * (1 - tff.admincost) * ax +
      capdth * (Ax1 + tff.costKO * axcost) +
      prem * (1 - tff.admincost) * (IAx1 + tff.costKO * Iax);
    break;
  case MIXED :
    value = cap * (Ex + 1.0/cm->X10 * tff.MIXEDPS * (Ax1 + tff.costKO * axcost)) -
      prem * (1 - tff.admincost) * ax;
    break;
  }
  
  return value;

}


