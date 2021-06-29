#include "actuarialfunctions.h"
#include "hashtable.h"

const double eps = 0.0000001;

double npx(register unsigned int lt, 
	register double ageX, register double ageXn, register int corr){ //lt = life table
    /* generally the rule is npx = lx(ageXn)/lx(ageX) but because lx has
       an integer value as its input we need to interpolate both these
       values
     */
    register double ip1; //interpolation value 1
    register double ip2; //interpolation value 2
    register int lxX; // alive at ageX
    register int lxX1; // alive at ageX + 1
    register int lxXn; // alive at ageXn 
    register int lxXn1; // alive at ageXn + 1 
    ageX += corr;
    ageXn += corr;
    ageX = fmax(0, ageX);
    ageXn = fmax(0, ageXn);
    lxX = lx(lt, ageX);
    lxX1 = lx(lt, ageX + 1);
    lxXn = lx(lt, ageXn);
    lxXn1 = lx(lt, ageXn + 1);

    ip1 = lxX - (ageX - floor(ageX)) * (lxX - lxX1);
    ip2 = lxXn - (ageXn - floor(ageXn)) * (lxXn - lxXn1);

    return ip2/ip1;
}

double nEx(unsigned int lt, double i, double charge, double ageX, double ageXn, int corr){
    double im = (1+i)/(1+charge) - 1;
    double n = ageXn - ageX;
    double vn = 1/pow(1 + im, n);
    double nPx = npx(lt, ageX, ageXn, corr);
    return vn * nPx;
}

double axn(unsigned int lt, double i, double charge, int prepost, int term,
	double ageX, double ageXn, int corr){
    static Hashtable *axntable; /* This is used so that axn gets saved 
				   and doesn't have to be calculated
				   over and over for the same arguments. */

    char key[256]; /* This is used to search the Hashtable to see whether or not it 
		      has already been calculated */
    snprintf(key, sizeof(key), "%d%f%f%d%d%f%f%d", lt, i, charge, prepost, term, ageX, ageXn, corr); 

    /* I took 5 * 3 * (12 * 45)^2 * 2 as a rough estimate of the amount of combinations for keys then times 1.3 and then the next prime number*/
    if (axntable == NULL)
	axntable = newHashtable(11372401, 1);

    List *h;

    if ((h = lookup(key, NULL, axntable)) == NULL) {
	if (12 % term != 0) {
	    printf("An incorrect term was chosen, payments are usually monthly (term = 12)\n");
	    printf("but can also be yearly for example (term = 1). term should be divisible\n");
	    printf("by 12 but \nterm = %d.\n", term);
	    exit(0);
	}
	else if (ageX > ageXn + eps) {
	    printf("warning: ageXn = %.6f < ageX = %.6f\n", ageXn, ageX);
	    printf("axn = 0 is taken in this case.\n");
	    h = lookup(key, "0", axntable);
	    return 0;
	}
	else {
	    double ageXk = ageX + (double)prepost/term; // current age in while loop
	    int payments = (int)((ageXn - ageX) * term + eps);
	    double value = 0; // return value
	    char valuestr[128]; // return value as string to input in hashtable
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
	    snprintf(valuestr, sizeof(valuestr), "%f", value);
	    h = lookup(key, valuestr, axntable);
	}
    }
    return atof(h->value);
}

double Ax1n(unsigned int lt, double i, double charge, double ageX, double ageXn, int corr){
    if (ageX > ageXn + eps) {
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

double IAx1n(unsigned int lt, double i, double charge, double ageX, double ageXn, int corr){
    if (ageX > ageXn + eps) {
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
double Iaxn(unsigned int lt, double i, double charge, int prepost, int term,
	double ageX, double ageXn, int corr){
    if (1 % term != 0) {
	printf("Iaxn has only been implemented for term = 1. You will need to adjust your \n");
	printf("input for term, at the moment term = %d.\n", term);
	exit(0);
    }
    else if (ageX > ageXn + eps) {
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
    double i;
    double Ex;
    CalcInput *CI = malloc(sizeof(CalcInput));

    //---PUC RESERVES---
    CI->res = cm->RES[PUC][EREE][gen][k];
    CI->prem = cm->PREMIUM[EREE][gen][k];
    CI->deltacap = cm->DELTACAP[EREE][k];
    CI->capdth = cm->CAPDTH[EREE][gen][k];
    CI->age = cm->age[k];
    CI->lt = &tff.ltINS[EREE][gen];
    CI->RA = (k+1 > MAXPROJBEFOREPROL ? NRA(cm, k+1) : cm->NRA);
    CI->cap = calcCAP(cm, CI);

    CI->capdth = cm->CAPDTH[EREE][gen][k+1];
    CI->age = cm->age[k+1];

    cm->RES[PUC][EREE][gen][k+1] = calcRES(cm, CI);

    //-  Capital life  -
    cm->CAP[EREE][gen][k] = CI->cap;

    //-  RESERVES PROFIT SHARING  -
    CI->res = cm->RESPS[PUC][EREE][gen][k];
    CI->prem = 0;
    CI->deltacap = 0;
    CI->capdth = 0;
    CI->age = cm->age[k];
    CI->cap = calcCAP(cm, CI);

    CI->age = cm->age[k+1];

    cm->RESPS[PUC][EREE][gen][k+1] = calcRES(cm, CI);
    cm->CAPPS[EREE][gen][k] = CI->cap;

    //-  Reduced Capital  -
    CI->res = cm->RES[PUC][EREE][gen][k+1];
    CI->prem = 0;
    CI->deltacap = cm->DELTACAP[EREE][k+1];
    CI->capdth = cm->CAPDTH[EREE][gen][k+1];
    CI->age = cm->age[k+1];
    CI->lt = &tff.ltAfterTRM[EREE][gen];
    CI->cap = calcCAP(cm, CI);

    CI->res = CI->cap;
    CI->prem = 0;
    CI->deltacap = 0;
    CI->age = CI->RA;
    CI->RA = NRA(cm, k+1);
    CI->lt = &tff.ltProlongAfterTRM[EREE];
    cm->REDCAP[PUC][EREE][gen][k+1] = calcCAP(cm, CI);

    // Profit sharing
    CI->res = cm->RESPS[PUC][EREE][gen][k+1];
    CI->prem = 0;
    CI->deltacap = 0;
    CI->capdth = 0;
    CI->age = cm->age[k+1];
    CI->lt = &tff.ltAfterTRM[EREE][gen];
    CI->RA = (k+1 > MAXPROJBEFOREPROL ? NRA(cm, k+1) : cm->NRA);
    CI->cap = calcCAP(cm, CI);

    CI->res = CI->cap;
    CI->age = CI->RA;
    CI->RA = NRA(cm, k+1);
    CI->lt = &tff.ltProlongAfterTRM[EREE];
    cm->REDCAP[PUC][EREE][gen][k+1] += calcCAP(cm, CI);

    //---TUC RESERVES---
    CI->RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
    i = cm->TAUX[EREE][gen];
    Ex = nEx(tff.ltINS[EREE][gen].lt, i, tff.costRES, CI->RA, min(2, NRA(cm, k), cm->NRA), 0);

    CI->res = cm->RES[PUC][EREE][gen][1];
    CI->prem = 0;
    CI->deltacap = 0;
    CI->capdth = cm->CAPDTH[EREE][gen][1];
    CI->age = cm->age[1];
    CI->lt = &tff.ltINS[EREE][gen];

    CI->cap = calcCAP(cm, CI) + cm->DELTACAP[EREE][0] * (CI->RA - cm->age[1]) * 12 * Ex;
    CI->age = cm->age[k+1];
    CI->lt = &tff.ltProlong[EREE];
    cm->RES[TUC][EREE][gen][k+1] = calcCAP(cm, CI);

    //-  RESERVES PROFIT SHARING  -
    cm->RESPS[TUC][EREE][gen][k+1] = cm->RESPS[PUC][EREE][gen][k+1];

    //-  Reduced Capital  -
    if (cm->tariff == MIXED || cm->tariff == UKMT) {
	CI->RA = min(2, NRA(cm, k+1), cm->NRA);
	CI->age = cm->age[1];
	CI->lt = &tff.ltAfterTRM[EREE][gen];
	CI->cap = calcCAP(cm, CI) + cm->DELTACAP[EREE][0] * (cm->NRA - cm->age[1]) * 12;

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = cm->NRA;
	CI->lt = &tff.ltProlongAfterTRM[EREE];
	cm->REDCAP[TUC][EREE][gen][k+1] = calcCAP(cm, CI);

	// Profit sharing
	CI->res = cm->RESPS[PUC][EREE][gen][1];
	CI->capdth = 0;
	CI->age = cm->age[1];
	CI->RA = min(2, NRA(cm, k+1), cm->NRA);
	CI->lt = &tff.ltAfterTRM[EREE][gen];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = cm->NRA;
	CI->lt = &tff.ltProlongAfterTRM[EREE];
	cm->REDCAP[TUC][EREE][gen][k+1] += calcCAP(cm, CI);    
    }
    else {
	CI->res = cm->RES[PUC][EREE][gen][1];
	CI->capdth = cm->CAPDTH[EREE][gen][1];
	CI->age = cm->age[1];
	CI->RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
	CI->lt = &tff.ltINS[EREE][gen];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = min(2, NRA(cm, k+1), cm->NRA);
	CI->lt = &tff.ltAfterTRM[EREE][gen];
	CI->cap = calcCAP(cm, CI) + cm->DELTACAP[EREE][0] * (cm->NRA - cm->age[1]) * 12;

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]);
	CI->lt = &tff.ltProlong[EREE];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = NRA(cm, k+1);
	CI->lt = &tff.ltProlongAfterTRM[EREE];
	cm->REDCAP[TUC][EREE][gen][k+1] = calcCAP(cm, CI);

	// Profit sharing
	CI->res = cm->RESPS[PUC][EREE][gen][1];
	CI->capdth = 0;
	CI->age = cm->age[1];
	CI->RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
	CI->lt = &tff.ltINS[EREE][gen];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = min(2, NRA(cm, k+1), cm->NRA);
	CI->lt = &tff.ltAfterTRM[EREE][gen];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]);
	CI->lt = &tff.ltProlong[EREE];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = NRA(cm, k+1);
	CI->lt = &tff.ltProlongAfterTRM[EREE];
	cm->REDCAP[TUC][EREE][gen][k+1] += calcCAP(cm, CI);
    }
    //---TUCPS_1 RESERVES---
    CI->res = cm->RES[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)],
    CI->age = cm->age[(int)min(2, (double)k+1, 2.0)];
    CI->prem = 0;
    CI->deltacap = 0;
    CI->capdth = cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)];
    CI->RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
    CI->lt = &tff.ltINS[EREE][gen];
    CI->cap = calcCAP(cm, CI) + cm->DELTACAP[EREE][0] * (CI->RA - CI->age) * 12 * Ex;

    CI->res = CI->cap;
    CI->capdth = cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)];
    CI->age = CI->RA;
    CI->RA = cm->age[k+1];
    CI->lt = &tff.ltProlong[EREE];

    cm->RES[TUCPS_1][EREE][gen][k+1] = calcCAP(cm, CI);

    //-  RESERVES PROFIT SHARING  -
    cm->RESPS[TUCPS_1][EREE][gen][k+1] = cm->RESPS[PUC][EREE][gen][k+1];

    //-  Reduced Capital  -
    if (cm->tariff == MIXED || cm->tariff == UKMT) {
	CI->RA = min(2, NRA(cm, k+1), cm->NRA);
	CI->res = cm->RES[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)];
	CI->prem = 0;
	CI->deltacap = 0;
	CI->capdth = cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)];
	CI->age = cm->age[(int)min(2, (double)k+1, 2.0)]; 
	CI->lt = &tff.ltAfterTRM[EREE][gen];
	CI->cap = calcCAP(cm, CI) + cm->DELTACAP[EREE][0] * (cm->NRA - CI->age) * 12;

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = cm->NRA;
	CI->lt = &tff.ltProlongAfterTRM[EREE];
	cm->REDCAP[TUCPS_1][EREE][gen][k+1] = calcCAP(cm, CI);

	// Profit sharing
	CI->res = cm->RESPS[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)];
	CI->capdth = 0;
	CI->age = cm->age[(int)min(2, (double)k+1, 2.0)]; 
	CI->RA = min(2, NRA(cm, k+1), cm->NRA);
	CI->lt = &tff.ltAfterTRM[EREE][gen];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = cm->NRA;
	CI->lt = &tff.ltProlongAfterTRM[EREE];
	cm->REDCAP[TUCPS_1][EREE][gen][k+1] += calcCAP(cm, CI);    
    }
    else {
	CI->RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
	CI->res = cm->RES[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)];
	CI->prem = 0;
	CI->deltacap = 0;
	CI->capdth = cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)];
	CI->age = cm->age[(int)min(2, (double)k+1, 2.0)]; 
	CI->lt = &tff.ltAfterTRM[EREE][gen];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = min(2, NRA(cm, k+1), cm->NRA);
	CI->cap = calcCAP(cm, CI) + cm->DELTACAP[EREE][0] * (cm->NRA - CI->age) * 12;

	CI->res = CI->cap;
	CI->age = min(2, NRA(cm, k+1), cm->NRA);
	CI->RA = max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]);
	CI->lt = &tff.ltProlong[EREE];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = NRA(cm, k+1);
	CI->lt = &tff.ltProlongAfterTRM[EREE];
	cm->REDCAP[TUCPS_1][EREE][gen][k+1] = calcCAP(cm, CI);

	// Profit sharing
	CI->res = cm->RESPS[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)];
	CI->capdth = 0;
	CI->age = cm->age[(int)min(2, (double)k+1, 2.0)]; 
	CI->RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
	CI->lt = &tff.ltINS[EREE][gen];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = min(2, NRA(cm, k+1), cm->NRA);
	CI->lt = &tff.ltAfterTRM[EREE][gen];
	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]);
	CI->lt = &tff.ltProlong[EREE];

	CI->cap = calcCAP(cm, CI);

	CI->res = CI->cap;
	CI->age = CI->RA;
	CI->RA = NRA(cm, k+1);
	CI->lt = &tff.ltProlongAfterTRM[EREE];
	cm->REDCAP[TUCPS_1][EREE][gen][k+1] += calcCAP(cm, CI);
    }
}

void evolART24(CurrentMember *cm, int k) {

    double i;
    double im;
    double admincost;
    double premium;

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

double calcCAP(CurrentMember *cm, CalcInput *CI)
{
    double res = CI->res, prem = CI->prem, deltacap = CI->deltacap, 
	   capdth = CI->capdth, age = CI->age, RA = CI->RA;
    LifeTable *lt = CI->lt;

    double i;
    double Ex;
    double ax;

    // These are used for UKMT and MIXED
    double axcost = 0;
    double Ax1 = 0;

    // These are used for UKMT
    double IAx1 = 0;
    double Iax = 0;

    double value;

    i = lt->i;
    Ex = nEx(lt->lt, i, tff.costRES, age, RA, 0);
    ax = axn(lt->lt, i, tff.costRES, tff.prepost, tff.term, age, RA, 0);

    switch(cm->tariff)
    {
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
	    axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
	    Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
	    IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
	    Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);

	    value = (res + prem * (1 - tff.admincost) * ax -
		    capdth * (Ax1 + tff.costKO * axcost) -
		    prem * (1 - tff.admincost) * (IAx1 + tff.costKO * Iax)) / Ex;
	    break;
	case MIXED :
	    axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
	    Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);

	    value = (res + prem * (1 - tff.admincost) * ax) /
		(Ex + 1.0/cm->X10 * tff.MIXEDPS * (Ax1 + tff.costKO * axcost));
	    break;
	default :
	    printf("Error in %s: %d is not a valid tariff.\n", __func__, cm->tariff);
	    exit(1);
    }

    return value;
}

double calcRES(CurrentMember *cm, CalcInput *CI)
{
    double cap = CI->cap, prem = CI->prem, deltacap = CI->deltacap, 
	   capdth = CI->capdth, age = CI->age, RA = CI->RA;
    LifeTable *lt = CI->lt;
    double i;
    double Ex;
    double ax;

    // These are used for UKMT and MIXED
    double axcost = 0; 
    double Ax1 = 0;

    // These are used for UKMT
    double IAx1 = 0;
    double Iax = 0;

    double value;

    i = lt->i;
    Ex = nEx(lt->lt, i, tff.costRES, age, RA, 0);
    ax = axn(lt->lt, i, tff.costRES, tff.prepost, tff.term, age, RA, 0);

    switch(cm->tariff)
    {
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
	    axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
	    Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
	    IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
	    Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);

	    value = cap * Ex - prem * (1 - tff.admincost) * ax +
		capdth * (Ax1 + tff.costKO * axcost) +
		prem * (1 - tff.admincost) * (IAx1 + tff.costKO * Iax);
	    break;
	case MIXED :
	    axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
	    Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);

	    value = cap * (Ex + 1.0/cm->X10 * tff.MIXEDPS * (Ax1 + tff.costKO * axcost)) -
		prem * (1 - tff.admincost) * ax;
	    break;
	default :
	    printf("Error in %s: %d is not a valid tariff.\n", __func__, cm->tariff);
	    exit(1);
    }
    return value;
}

void evolDBONCIC(CurrentMember *cm, int k, double ART24TOT[], double RESTOT[], double REDCAPTOT[]) {

    double probfactdef; // probability factors for deferred payment
    double probfactimm; // probability factors for immediate payment
    double amountdef;
    double amountimm;

    probfactdef = cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k];
    probfactimm = (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];

    // DBO, NC, IC
    for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 3; j++) {
	    amountdef = getamount(cm, k, DBO, i, j, DEF, PBO, ART24TOT, RESTOT, REDCAPTOT);
	    amountimm = getamount(cm, k, DBO, i, j, IMM, PBO, ART24TOT, RESTOT, REDCAPTOT);
	    cm->DBORET[i][j][k] = amountdef * probfactdef + amountimm * probfactimm;

	    amountdef = getamount(cm, k, NC, i, j, DEF, PBO, ART24TOT, RESTOT, REDCAPTOT);
	    amountimm = getamount(cm, k, NC, i, j, IMM, PBO, ART24TOT, RESTOT, REDCAPTOT);
	    cm->NCRET[i][j][k] = amountdef * probfactdef + amountimm * probfactimm;

	    amountdef = getamount(cm, k, IC, i, j, DEF, PBO, ART24TOT, RESTOT, REDCAPTOT);
	    amountimm = getamount(cm, k, IC, i, j, IMM, PBO, ART24TOT, RESTOT, REDCAPTOT);
	    cm->ICNCRET[i][j][k] = amountdef * probfactdef + amountimm * probfactimm;
	}
	amountdef = getamount(cm, k, ASSETS, 0, PAR113*i, DEF, PBO, ART24TOT, RESTOT, REDCAPTOT);
	amountimm = getamount(cm, k, ASSETS, 0, PAR113*i, IMM, PBO, ART24TOT, RESTOT, REDCAPTOT);
	cm->assets[PAR113*i][k] = amountdef * probfactdef + amountimm * probfactimm;
    }	
}

void evolEBP(CurrentMember *cm, int k, 
	double ART24TOT[], double RESTOT[], double REDCAPTOT[]) {

    double probfactdef; // probability factors for deferred payment
    double probfactimm; // probability factors for immediate payment
    double amountdef;
    double amountimm;
    int yearIMM; // year of immediate payment	
    int yearDEF; // year of deferred payment	
    double vIMM;
    double vDEF;

    yearIMM = calcyears(cm->DOC[1], cm->DOC[k], 0);
    yearDEF = max(2, 0.0, calcyears(cm->DOC[1], newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1), 0));

    //-  EBP yearIMM  -
    if (yearIMM >= 0) {
	vIMM = pow(1 + ass.DR, -calcyears(newDate(0, cm->DOC[1]->year + yearIMM, cm->DOC[1]->month, 1), cm->DOC[k], 0));
	probfactimm = (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * vIMM;	

	for (int i = 0; i < 2; i++) { // PUCTUC
	    for (int j = 0; j < 3; j++) { // PUCTUC
		for (int l = 0; l < 2; l++) { // PBOTBO 
		    amountimm = getamount(cm, k, DBO, i, j, IMM, l, ART24TOT, RESTOT, REDCAPTOT);
		    cm->EBP[i][j][l][yearIMM+1] += amountimm * probfactimm;
		}
		amountimm = getamount(cm, k, NC, i, j, IMM, PBO, ART24TOT, RESTOT, REDCAPTOT);
		cm->PBONCCF[i][j][yearIMM+1] += amountimm * probfactimm;
	    }
	}
	cm->EBPDTH[TBO][yearIMM+1] += (cm->CAPDTHRiskPart[k] + cm->CAPDTHRESPart[k]) * 
	    cm->qx[k] * cm->kPx[k] * vIMM;
	cm->EBPDTH[PBO][yearIMM+1] += (cm->CAPDTHRiskPart[k] + cm->CAPDTHRESPart[k]) * 
	    cm->FF[k] * cm->qx[k] * cm->kPx[k] * vIMM;
	cm->PBODTHNCCF[yearIMM+1] += (cm->CAPDTHRiskPart[k] + cm->CAPDTHRESPart[k]) * 
	    cm->FFSC[k] * cm->qx[k] * cm->kPx[k] * vIMM;
    }

    //-  EBP yearDEF  -
    vDEF = pow(1 + ass.DR, -calcyears(newDate(0, cm->DOC[1]->year + yearDEF, cm->DOC[1]->month, 1),
		newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1), 0));	
    probfactdef = cm->wxdef[k] * cm->nPk[k] * cm->kPx[k] * vDEF;	

    for (int i = 0; i < 2; i++) { // PUCTUC
	for (int j = 0; j < 3; j++) { // PUCTUC
	    for (int l = 0; l < 2; l++) { // PBOTBO 
		amountdef = getamount(cm, k, DBO, i, j, DEF, l, ART24TOT, RESTOT, REDCAPTOT);
		cm->EBP[i][j][l][yearDEF+1] += amountdef * probfactdef;
	    }
	    amountdef = getamount(cm, k, NC, i, j, DEF, PBO, ART24TOT, RESTOT, REDCAPTOT);
	    cm->PBONCCF[i][j][yearDEF+1] += amountdef * probfactdef;
	}
    }
}

double getamount(CurrentMember *cm,  int k,  
	unsigned short DBONCICASS,  
	unsigned short method,  
	unsigned short assets,  
	unsigned short DEFIMM, 
	unsigned short PBOTBO,
	double ART24TOT[], double RESTOT[], double REDCAPTOT[]) {

    switch (DBONCICASS) {
	case DBO :
	    switch (method) {
		case PUC :
		    switch (assets) {
			case PAR113 :
			case PAR115 :
			    switch (DEFIMM) {
				case DEF : return max(2, ART24TOT[PUC] * pow(cm->FF[k], 1 - PBOTBO), REDCAPTOT[TUC]); 
				case IMM : return max(2, ART24TOT[PUC] * pow(cm->FF[k], 1 - PBOTBO), RESTOT[TUC]);
				default : printf("Error in %s: DEFIMM = %d\n", __func__, DEFIMM);
					  exit(1);
			    }
			case MATHRES : return ART24TOT[PUC] * pow(cm->FF[k], 1 - PBOTBO);
			default : printf("Error in %s: assets = %d\n", __func__, assets);
				  exit(1);
		    }
		case TUC :
		    switch (assets) {
			case PAR113 :
			case PAR115 :
			    switch (DEFIMM) {
				case DEF : return max(2, ART24TOT[TUC], REDCAPTOT[TUC]);
				case IMM : return max(2, ART24TOT[TUC], RESTOT[TUC]); 
				default : printf("Error in %s: DEFIMM = %d\n", __func__, DEFIMM);
					  exit(1);
			    }
			case MATHRES : return ART24TOT[TUC];
			default : printf("Error in %s: assets = %d\n", __func__, assets);
				  exit(1);
		    }
		default : printf("Error in %s: method = %d\n", __func__, method);
			  exit(1);
	    }
	case NC :
	    switch (method) {
		case PUC : return ART24TOT[PUC] * cm->FFSC[k];
		case TUC : return (ART24TOT[TUCPS_1] - ART24TOT[TUC]);
		default : printf("Error in %s: method = %d\n", __func__, method);
			  exit(1);
	    }
	case IC :
	    switch (method) {
		case PUC : return ART24TOT[PUC] * cm->FFSC[k] * ass.DR;
		case TUC : return (ART24TOT[TUCPS_1] - ART24TOT[TUC]) * ass.DR;
		default : printf("Error in %s: method = %d\n", __func__, method);
			  exit(1);
	    }
	case ASSETS :
	    switch (assets) {
		case PAR113 :
		    switch (DEFIMM) {
			case DEF : return REDCAPTOT[TUC] / cm->vn[k] * cm->vn113[k];
			case IMM : return RESTOT[TUC] / cm->vk[k] * cm->vk113[k];
			default : printf("Error in %s: DEFIMM = %d\n", __func__, DEFIMM);
				  exit(1);
		    }
		case PAR115 :
		    switch (DEFIMM) {
			case DEF : return REDCAPTOT[TUC];
			case IMM : return RESTOT[TUC];
			default : printf("Error in %s: DEFIMM = %d\n", __func__, DEFIMM);
				  exit(1);
		    }
		default :
		    printf("Error in %s: assets = %d\n", __func__, assets);
		    exit(1);

	    }
	default :
	    printf("Error in %s: DBONCICASS = %d\n", __func__, DBONCICASS);
	    exit(1);
    }
}
