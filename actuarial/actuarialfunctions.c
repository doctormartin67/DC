#include "actuarialfunctions.h"
#include "hashtable.h"

const double eps = 0.0000001;

static Hashtable *axntable; /* This is used so that axn gets saved 
			       and doesn't have to be calculated
			       over and over for the same arguments. */

double npx(unsigned int lt, double ageX, double ageXn, int corr){ //lt = life table
    /* generally the rule is npx = lx(ageXn)/lx(ageX) but because lx has
       an integer value as its input we need to interpolate both these
       values
     */

    double ip1; //interpolation value 1
    double ip2; //interpolation value 2
    int lxX; // alive at ageX
    int lxX1; // alive at ageX + 1
    int lxXn; // alive at ageXn 
    int lxXn1; // alive at ageXn + 1 
    ageX += corr;
    ageXn += corr;
    ageX = fmax(0, ageX);
    ageXn = fmax(0, ageXn);
    lxX = lx(lt, ageX);
    lxX1 = lx(lt, ageX + 1);
    lxXn = lx(lt, ageXn);
    lxXn1 = lx(lt, ageXn + 1);

    ip1 = lxX - (ageX - (int)ageX) * (lxX - lxX1);
    ip2 = lxXn - (ageXn - (int)ageXn) * (lxXn - lxXn1);

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
    char key[256]; // This is used to search the Hashtable to see whether or not is has already been calculated
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
    double cap;
    double RA;
    double i;
    double Ex;

    //---PUC RESERVES---
    RA = (k+1 > MAXPROJBEFOREPROL ? NRA(cm, k+1) : cm->NRA);

    cap = calcCAP(cm, 
	    cm->RES[PUC][EREE][gen][k],
	    cm->PREMIUM[EREE][gen][k],
	    cm->DELTACAP[EREE][k],
	    cm->CAPDTH[EREE][gen][k], cm->age[k], RA, &tff.ltINS[EREE][gen]);
    cm->RES[PUC][EREE][gen][k+1] = calcRES(cm, k+1,
	    cap,
	    cm->PREMIUM[EREE][gen][k],
	    cm->DELTACAP[EREE][k],
	    cm->CAPDTH[EREE][gen][k+1],
	    cm->age[k+1], &tff.ltINS[EREE][gen]);
    //-  Capital life  -
    cm->CAP[EREE][gen][k] = cap;

    //-  RESERVES PROFIT SHARING  -
    cap = calcCAP(cm, 
	    cm->RESPS[PUC][EREE][gen][k],
	    0, 0, 0,
	    cm->age[k], RA, &tff.ltINS[EREE][gen]);
    cm->RESPS[PUC][EREE][gen][k+1] = calcRES(cm, k+1,
	    cap,
	    0, 0, 0,
	    cm->age[k+1], &tff.ltINS[EREE][gen]);
    cm->CAPPS[EREE][gen][k] = cap;

    //-  Reduced Capital  -
    RA = (k+1 > MAXPROJBEFOREPROL ? NRA(cm, k+1) : cm->NRA);
    cap = calcCAP(cm, 
	    cm->RES[PUC][EREE][gen][k+1],
	    0,
	    cm->DELTACAP[EREE][k+1],
	    cm->CAPDTH[EREE][gen][k+1], cm->age[k+1], RA, &tff.ltAfterTRM[EREE][gen]);
    cm->REDCAP[PUC][EREE][gen][k+1] = calcCAP(cm, 
	    cap,
	    0, 0,
	    cm->CAPDTH[EREE][gen][k+1],
	    RA, NRA(cm, k+1), &tff.ltProlongAfterTRM[EREE]);
    // Profit sharing
    cap = calcCAP(cm, 
	    cm->RESPS[PUC][EREE][gen][k+1],
	    0, 0, 0,
	    cm->age[k+1], RA, &tff.ltAfterTRM[EREE][gen]);
    cm->REDCAP[PUC][EREE][gen][k+1] += calcCAP(cm, 
	    cap,
	    0, 0, 0,
	    RA, NRA(cm, k+1), &tff.ltProlongAfterTRM[EREE]);

    //---TUC RESERVES---
    RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
    i = cm->TAUX[EREE][gen];
    Ex = nEx(tff.ltINS[EREE][gen].lt, i, tff.costRES, RA, min(2, NRA(cm, k), cm->NRA), 0);

    cap = calcCAP(cm, 
	    cm->RES[PUC][EREE][gen][1],
	    0, 0,
	    cm->CAPDTH[EREE][gen][1], cm->age[1], RA, &tff.ltINS[EREE][gen]) +
	cm->DELTACAP[EREE][0] * (RA - cm->age[1]) * 12 * Ex;
    cm->RES[TUC][EREE][gen][k+1] = calcCAP(cm, 
	    cap,
	    0, 0,
	    cm->CAPDTH[EREE][gen][1],
	    RA, cm->age[k+1], &tff.ltProlong[EREE]);

    //-  RESERVES PROFIT SHARING  -
    cm->RESPS[TUC][EREE][gen][k+1] = cm->RESPS[PUC][EREE][gen][k+1];

    //-  Reduced Capital  -
    if (cm->tariff == MIXED || cm->tariff == UKMT) {
	RA = min(2, NRA(cm, k+1), cm->NRA);
	cap = calcCAP(cm, 
		cm->RES[PUC][EREE][gen][1],
		0, 0,
		cm->CAPDTH[EREE][gen][1], cm->age[1], RA, &tff.ltAfterTRM[EREE][gen]) +
	    cm->DELTACAP[EREE][0] * (cm->NRA - cm->age[1]) * 12;
	cm->REDCAP[TUC][EREE][gen][k+1] = calcCAP(cm, 
		cap,
		0, 0,
		cm->CAPDTH[EREE][gen][1],
		RA, cm->NRA, &tff.ltProlongAfterTRM[EREE]);
	// Profit sharing
	cap = calcCAP(cm, 
		cm->RESPS[PUC][EREE][gen][1],
		0, 0, 0, cm->age[1], RA, &tff.ltAfterTRM[EREE][gen]);
	cm->REDCAP[TUC][EREE][gen][k+1] += calcCAP(cm, 
		cap,
		0, 0, 0,
		RA, cm->NRA, &tff.ltProlongAfterTRM[EREE]);    
    }
    else {
	cap = calcCAP(cm, 
		cm->RES[PUC][EREE][gen][1],
		0, 0,
		cm->CAPDTH[EREE][gen][1], cm->age[1], RA, &tff.ltINS[EREE][gen]);
	cap = calcCAP(cm, 
		cap, 0, 0,
		cm->CAPDTH[EREE][gen][1], RA, min(2, NRA(cm, k+1), cm->NRA),
		&tff.ltAfterTRM[EREE][gen]) +
	    cm->DELTACAP[EREE][0] * (cm->NRA - cm->age[1]) * 12;
	cap = calcCAP(cm, 
		cap, 0, 0,
		cm->CAPDTH[EREE][gen][1], min(2, NRA(cm, k+1), cm->NRA),
		max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]),
		&tff.ltProlong[EREE]);
	cm->REDCAP[TUC][EREE][gen][k+1] = calcCAP(cm, 
		cap,
		0, 0,
		cm->CAPDTH[EREE][gen][1],
		max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]),
		NRA(cm, k+1), &tff.ltProlongAfterTRM[EREE]);
	// Profit sharing
	cap = calcCAP(cm, 
		cm->RESPS[PUC][EREE][gen][1],
		0, 0, 0,
		cm->age[1], RA, &tff.ltINS[EREE][gen]);
	cap = calcCAP(cm, 
		cap, 0, 0, 0,
		RA, min(2, NRA(cm, k+1), cm->NRA),
		&tff.ltAfterTRM[EREE][gen]);
	cap = calcCAP(cm, 
		cap, 0, 0, 0,
		min(2, NRA(cm, k+1), cm->NRA),
		max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]),
		&tff.ltProlong[EREE]);
	cm->REDCAP[TUC][EREE][gen][k+1] += calcCAP(cm, 
		cap,
		0, 0, 0,
		max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]),
		NRA(cm, k+1), &tff.ltProlongAfterTRM[EREE]);
    }
    //---TUCPS_1 RESERVES---
    RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
    cap = calcCAP(cm, 
	    cm->RES[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)],
	    0, 0,
	    cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)],
	    cm->age[(int)min(2, (double)k+1, 2.0)], RA, &tff.ltINS[EREE][gen]) +
	cm->DELTACAP[EREE][0] * (RA - cm->age[(int)min(2, (double)k+1, 2.0)]) * 12 * Ex;
    cm->RES[TUCPS_1][EREE][gen][k+1] = calcCAP(cm, 
	    cap,
	    0, 0,
	    cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)],
	    RA, cm->age[k+1], &tff.ltProlong[EREE]);

    //-  RESERVES PROFIT SHARING  -
    cm->RESPS[TUCPS_1][EREE][gen][k+1] = cm->RESPS[PUC][EREE][gen][k+1];

    //-  Reduced Capital  -
    if (cm->tariff == MIXED || cm->tariff == UKMT) {
	RA = min(2, NRA(cm, k+1), cm->NRA);
	cap = calcCAP(cm, 
		cm->RES[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)],
		0, 0,
		cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)],
		cm->age[(int)min(2, (double)k+1, 2.0)], RA, &tff.ltAfterTRM[EREE][gen]) +
	    cm->DELTACAP[EREE][0] * (cm->NRA - cm->age[(int)min(2, (double)k+1, 2.0)]) * 12;
	cm->REDCAP[TUCPS_1][EREE][gen][k+1] = calcCAP(cm, 
		cap,
		0, 0,
		cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)],
		RA, cm->NRA, &tff.ltProlongAfterTRM[EREE]);
	// Profit sharing
	cap = calcCAP(cm, 
		cm->RESPS[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)],
		0, 0, 0, cm->age[(int)min(2, (double)k+1, 2.0)], RA, &tff.ltAfterTRM[EREE][gen]);
	cm->REDCAP[TUCPS_1][EREE][gen][k+1] += calcCAP(cm, 
		cap,
		0, 0, 0,
		RA, cm->NRA, &tff.ltProlongAfterTRM[EREE]);    
    }
    else {
	RA = min(3, NRA(cm, k+1), cm->NRA, cm->age[k+1]);
	cap = calcCAP(cm, 
		cm->RES[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)],
		0, 0,
		cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)],
		cm->age[(int)min(2, (double)k+1, 2.0)], RA, &tff.ltINS[EREE][gen]);
	cap = calcCAP(cm, 
		cap, 0, 0,
		cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)],
		RA, min(2, NRA(cm, k+1), cm->NRA),
		&tff.ltAfterTRM[EREE][gen]) +
	    cm->DELTACAP[EREE][0] * (cm->NRA - cm->age[(int)min(2, (double)k+1, 2.0)]) * 12;
	cap = calcCAP(cm, 
		cap, 0, 0,
		cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)],
		min(2, NRA(cm, k+1), cm->NRA),
		max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]),
		&tff.ltProlong[EREE]);
	cm->REDCAP[TUCPS_1][EREE][gen][k+1] = calcCAP(cm, 
		cap,
		0, 0,
		cm->CAPDTH[EREE][gen][(int)min(2, (double)k+1, 2.0)],
		max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]),
		NRA(cm, k+1), &tff.ltProlongAfterTRM[EREE]);
	// Profit sharing
	cap = calcCAP(cm, 
		cm->RESPS[PUC][EREE][gen][(int)min(2, (double)k+1, 2.0)],
		0, 0, 0,
		cm->age[(int)min(2, (double)k+1, 2.0)], RA, &tff.ltINS[EREE][gen]);
	cap = calcCAP(cm, 
		cap, 0, 0, 0,
		RA, min(2, NRA(cm, k+1), cm->NRA),
		&tff.ltAfterTRM[EREE][gen]);
	cap = calcCAP(cm, 
		cap, 0, 0, 0,
		min(2, NRA(cm, k+1), cm->NRA),
		max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]),
		&tff.ltProlong[EREE]);
	cm->REDCAP[TUCPS_1][EREE][gen][k+1] += calcCAP(cm, 
		cap,
		0, 0, 0,
		max(2, min(2, NRA(cm, k+1), cm->NRA), cm->age[k+1]),
		NRA(cm, k+1), &tff.ltProlongAfterTRM[EREE]);
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

double calcCAP(CurrentMember *cm, 
	double res, double prem, double deltacap, double capdth,
	double age, double RA, LifeTable *lt) {

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

double calcRES(CurrentMember *cm, int k,
	double cap, double prem, double deltacap, double capdth,
	double age, LifeTable *lt) {

    double RA;
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

    RA = (k > MAXPROJBEFOREPROL ? NRA(cm, k) : cm->NRA);
    i = lt->i;
    Ex = nEx(lt->lt, i, tff.costRES, age, RA, 0);
    ax = axn(lt->lt, i, tff.costRES, tff.prepost, tff.term, age, RA, 0);

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
				case DEF :
				    return max(2, ART24TOT[PUC] * pow(cm->FF[k], 1 - PBOTBO), REDCAPTOT[TUC]); 
				case IMM :
				    return max(2, ART24TOT[PUC] * pow(cm->FF[k], 1 - PBOTBO), RESTOT[TUC]);
				default :
				    printf("ERROR: DEFIMM = %d, but expected %d or %d\n", DEFIMM, DEF, IMM);
				    exit(1);
			    }
			case MATHRES :
			    return ART24TOT[PUC] * pow(cm->FF[k], 1 - PBOTBO);
			default :
			    printf("ERROR: assets = %d, but expected %d, %d or %d\n", assets, PAR113, PAR115, MATHRES);
			    exit(1);
		    }
		case TUC :
		    switch (assets) {
			case PAR113 :
			case PAR115 :
			    switch (DEFIMM) {
				case DEF :
				    return max(2, ART24TOT[TUC], REDCAPTOT[TUC]);
				case IMM :
				    return max(2, ART24TOT[TUC], RESTOT[TUC]); 
				default :
				    printf("ERROR: DEFIMM = %d, but expected %d or %d\n", DEFIMM, DEF, IMM);
				    exit(1);
			    }
			case MATHRES :
			    return ART24TOT[TUC];
			default :
			    printf("ERROR: assets = %d, but expected %d, %d or %d\n", assets, PAR113, PAR115, MATHRES);
			    exit(1);
		    }
		default :
		    printf("ERROR: method = %d, but expected %d or %d\n", method, PUC, TUC);
		    exit(1);
	    }
	case NC :
	    switch (method) {
		case PUC :
		    return ART24TOT[PUC] * cm->FFSC[k];
		case TUC :
		    return (ART24TOT[TUCPS_1] - ART24TOT[TUC]);
		default :
		    printf("ERROR: method = %d, but expected %d or %d\n", method, PUC, TUC);
		    exit(1);
	    }
	case IC :
	    switch (method) {
		case PUC :
		    return ART24TOT[PUC] * cm->FFSC[k] * ass.DR;
		case TUC :
		    return (ART24TOT[TUCPS_1] - ART24TOT[TUC]) * ass.DR;
		default :
		    printf("ERROR: method = %d, but expected %d or %d\n", method, PUC, TUC);
		    exit(1);
	    }
	case ASSETS :
	    switch (assets) {
		case PAR113 :
		    switch (DEFIMM) {
			case DEF :
			    return REDCAPTOT[TUC] / cm->vn[k] * cm->vn113[k];
			case IMM :
			    return RESTOT[TUC] / cm->vk[k] * cm->vk113[k];
			default :
			    printf("ERROR: DEFIMM = %d, but expected %d or %d\n", DEFIMM, DEF, IMM);
			    exit(1);
		    }
		case PAR115 :
		    switch (DEFIMM) {
			case DEF :
			    return REDCAPTOT[TUC];
			case IMM :
			    return RESTOT[TUC];
			default :
			    printf("ERROR: DEFIMM = %d, but expected %d or %d\n", DEFIMM, DEF, IMM);
			    exit(1);
		    }
		default :
		    printf("ERROR: assets = %d, but expected %d or %d\n", assets, PAR113, PAR115);
		    exit(1);

	    }
	default :
	    printf("ERROR: DBONCICASS = %d, but expected %d, %d, %d or %d\n", DBONCICASS, DBO, NC, IC, ASSETS);
	    exit(1);
    }
}
