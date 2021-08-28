#include "libraryheader.h"
#include "dates.h"
#include "actuarialfunctions.h"
#include "hashtable.h"
#include "errorexit.h"

const double eps = 0.0000001;

static void updateART24ACT(CurrentMember *cm, int k);
static void updateART24reserves(unsigned method, CurrentMember *cm,
		unsigned EREE, unsigned ART24gen, int k);
static double get_premium_method(unsigned method, CurrentMember *cm,
		unsigned EREE, int k);
static void updateART24DEF(CurrentMember *cm, int k);
static double getDBO(const CurrentMember *cm, int k, unsigned method,
		unsigned assets, unsigned DEFIMM, unsigned PBOTBO, 
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT]);
static double getAssets(const CurrentMember *cm, int k, unsigned assets,
		unsigned DEFIMM,
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT]);
static void updateRESCAP(CurrentMember *cm, int k);
static void updateRESCAPPS(CurrentMember *cm, int k);
static void updateREDCAPPUC(CurrentMember *cm, int k);
static void updateRESTUC(CurrentMember *cm, int k);
static void updateREDCAPTUC(CurrentMember *cm, int k);
static void updateRESTUCPS_1(CurrentMember *cm, int k);
static void updateREDCAPTUCPS_1(CurrentMember *cm, int k);

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

void evolCAPDTH(CurrentMember *cm, int k) {
	for (int i = 0; i < EREE_AMOUNT; i++) {
		cm->DELTACAP[i][k+1] = cm->DELTACAP[i][k];
		for (int gen = 0; gen < MAXGEN; gen++) {
			cm->CAPDTH[i][gen][k+1] = cm->CAPDTH[i][gen][k]
				+ cm->PREMIUM[i][gen][k]
				* (1 - tff.admincost)
				* (cm->age[k+1] - cm->age[k]);
		}
	}
}

void evolRES(CurrentMember *cm, int k)
{
	updateRESCAP(cm, k);
	updateRESCAPPS(cm, k);
	updateREDCAPPUC(cm, k);
	updateRESTUC(cm, k);
	updateREDCAPTUC(cm, k);
	updateRESTUCPS_1(cm, k);
	updateREDCAPTUCPS_1(cm, k);
}

void evolPremiums(CurrentMember *cm, int k)
{
	double formula = 0.0;
	double prem = 0.0;
	double Ax1 = 0.0;
	double ax = 0.0;
	double axcost = 0.0;
	double RPcap = 0.0;
	double RPres = 0.0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		formula = (ER == i ? calcA(cm, k + 1) : calcC(cm, k + 1));
		for (int j = 0; j < MAXGEN; j++) {
			prem = formula;
			for (int l = 0; l < j; l++)
				prem = prem - cm->PREMIUM[i][l][k+1];
			prem = (j < MAXGEN - 1 ? min(2, cm->PREMIUM[i][j][k]
						, prem) : prem);

			cm->PREMIUM[i][j][k+1] = prem;

			if (cm->age[k+1] == cm->age[k] 
					|| (cm->tariff != MIXED))
				cm->RP[i][j][k] = 0;
			else {
				Ax1 = Ax1n(tff.ltINS[i][j].lt, cm->TAUX[i][j],
						tff.costRES, cm->age[k], 
						cm->age[k+1], 0);
				ax = axn(tff.ltINS[i][j].lt, cm->TAUX[i][j],
						tff.costRES, tff.prepost, 
						tff.term, cm->age[k], 
						cm->age[k+1], 0);
				axcost = axn(tff.ltINS[i][j].lt, 
						cm->TAUX[i][j], tff.costRES, 0,
						1, cm->age[k], 
						cm->age[k+1], 0);
				RPcap = cm->CAP[i][j][k]/cm->X10;
				RPres = cm->RES[PUC][i][j][k];

				cm->RP[i][j][k] = fmax(0.0, (RPcap - RPres)
						* Ax1/ax)
					+ RPcap * tff.costKO * axcost;
			}			     
		}
	}

}

void evolART24(CurrentMember *cm, int k)
{
	if (cm->status & ACT)
		updateART24ACT(cm, k);
	else
		updateART24DEF(cm, k);
}

static void updateART24ACT(CurrentMember *cm, int k)
{
	for (int l = 0; l < EREE_AMOUNT; l++) {
		for (int m = 0; m < ART24GEN_AMOUNT; m++) {
			for (int j = 0; j < METHOD_AMOUNT; j++) {
				updateART24reserves(j, cm, l, m, k);
			}
		}
	}
}

static void updateART24reserves(unsigned method, CurrentMember *cm,
		unsigned EREE, unsigned ART24gen, int k)
{
	unsigned pp = 0.0;
	double i = 0.0;
	double im = 0.0;
	double admincost = tff.admincost;
	double premium = 0.0;
	double rp = 0.0;
	double period = 0.0;

	i = ART24TAUX[EREE][ART24gen];
	period = cm->age[k+1] - cm->age[k];

	cm->ART24[method][EREE][ART24gen][k+1] = 
		cm->ART24[method][EREE][ART24gen][k] * pow(1 + i, period);

	if (ART24GEN1 == ART24gen)
		return;

	pp = (tff.prepost == 0 ? 1 : 0);
	admincost = (ER == EREE ? min(2, ART24admincost, admincost) : 0);
	rp = gensum(cm->RP, EREE, k);
	im = pow(1 + i, 1.0/tff.term) - 1;
	premium = get_premium_method(method, cm, EREE, k);
	premium = max(2, 0.0, premium * (1 - admincost) - rp);

	cm->ART24[method][EREE][ART24gen][k+1] += premium / tff.term *
		(pow(1 + im, tff.term * period + pp) - 1 - im * pp) / im;

}
static double get_premium_method(unsigned method, CurrentMember *cm,
		unsigned EREE, int k)
{
	double premium = 0.0;

	switch(method) {
		case PUC :
			premium = gensum(cm->PREMIUM, EREE, k);
			break;
		case TUC :
			premium = (k > 0 ? 0 : gensum(cm->PREMIUM, EREE, k));
			break;
		case TUCPS_1 :
			premium = (k > 1 ? 0 : gensum(cm->PREMIUM, EREE, k));
			break;
	}
	
	return premium;
}

static void updateART24DEF(CurrentMember *cm, int k)
{
	for (int j = 0; j < METHOD_AMOUNT; j++)
		for (int l = 0; l < EREE_AMOUNT; l++)
			for (int m = 0; m < ART24GEN_AMOUNT; m++)
				cm->ART24[j][l][m][k+1] = 
					cm->ART24[j][l][m][k]; 
}

double calcCAP(CurrentMember *cm, CalcInput *CI)
{
	double res = CI->res, prem = CI->prem, deltacap = CI->deltacap, 
	       capdth = CI->capdth, age = CI->age, RA = CI->RA;
	LifeTable *lt = CI->lt;

	double i = 0.0;
	double Ex = 0.0;
	double ax = 0.0;

	// These are used for UKMT and MIXED
	double axcost = 0.0;
	double Ax1 = 0.0;

	// These are used for UKMT
	double IAx1 = 0.0;
	double Iax = 0.0;

	double value = 0.0;

	i = lt->i;
	Ex = nEx(lt->lt, i, tff.costRES, age, RA, 0);
	ax = axn(lt->lt, i, tff.costRES, tff.prepost, tff.term, age, RA, 0);

	switch(cm->tariff) {
		case UKMS :
		case UKZT :
			value = (res + prem * (1 - tff.admincost) * ax) / Ex
				+ deltacap * (RA - age) * 12;
			break;
		case UKMT :
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
			IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
			Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);

			value = (res + prem * (1 - tff.admincost) * ax
					- capdth * (Ax1 + tff.costKO * axcost)
					- prem * (1 - tff.admincost)
					* (IAx1 + tff.costKO * Iax)) / Ex;
			break;
		case MIXED :
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);

			value = (res + prem * (1 - tff.admincost) * ax)
				/ (Ex + 1.0/cm->X10 * tff.MIXEDPS
				* (Ax1 + tff.costKO * axcost));
			break;
		default :
			errExit("[%s] %d is not a valid tariff.\n", __func__, 
					cm->tariff);
	}

	return value;
}

double calcRES(CurrentMember *cm, CalcInput *CI)
{
	double cap = CI->cap, prem = CI->prem, deltacap = CI->deltacap, 
	       capdth = CI->capdth, age = CI->age, RA = CI->RA;
	LifeTable *lt = CI->lt;
	double i = 0.0;
	double Ex = 0.0;
	double ax = 0.0;

	// These are used for UKMT and MIXED
	double axcost = 0.0; 
	double Ax1 = 0.0;

	// These are used for UKMT
	double IAx1 = 0.0;
	double Iax = 0.0;

	double value = 0.0;

	i = lt->i;
	Ex = nEx(lt->lt, i, tff.costRES, age, RA, 0);
	ax = axn(lt->lt, i, tff.costRES, tff.prepost, tff.term, age, RA, 0);

	switch(cm->tariff) {
		case UKMS :
		case UKZT :
			value = (cap - deltacap * (RA - age) * 12) * Ex
				- prem * (1 - tff.admincost) * ax;
			break;
		case UKMT :
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
			IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
			Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);

			value = cap * Ex - prem * (1 - tff.admincost) * ax
				+ capdth * (Ax1 + tff.costKO * axcost)
				+ prem * (1 - tff.admincost)
				* (IAx1 + tff.costKO * Iax);
			break;
		case MIXED :
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);

			value = cap * (Ex + 1.0/cm->X10 * tff.MIXEDPS
					* (Ax1 + tff.costKO * axcost))
				- prem * (1 - tff.admincost) * ax;
			break;
		default :
			errExit("[%s] %d is not a valid tariff.\n", __func__, 
					cm->tariff);
	}

	return value;
}

void evolDBONCIC(CurrentMember *cm, int k, 
		double ART24TOT[const static METHOD_AMOUNT],
		double RESTOT[const static METHOD_AMOUNT],
		double REDCAPTOT[const static METHOD_AMOUNT])
{
	double probfactdef = 0.0; // probability factors for deferred payment
	double probfactimm = 0.0; // probability factors for immediate payment
	double amountdef = 0.0;
	double amountimm = 0.0;

	probfactdef = cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k];
	probfactimm = (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];

	for (int i = 0; i < METHOD_AMOUNT; i++) {
		if (PUC != i && TUC != i) continue;
		for (int j = 0; j < ASSET_AMOUNT; j++) {
			amountdef = getamount(cm, k, DBO, i, j, DEF, PBO,
					ART24TOT, RESTOT, REDCAPTOT);
			amountimm = getamount(cm, k, DBO, i, j, IMM, PBO,
					ART24TOT, RESTOT, REDCAPTOT);
			cm->DBORET[i][j][k] = amountdef * probfactdef
				+ amountimm * probfactimm;

			amountdef = getamount(cm, k, NC, i, j, DEF, PBO,
					ART24TOT, RESTOT, REDCAPTOT);
			amountimm = getamount(cm, k, NC, i, j, IMM, PBO,
					ART24TOT, RESTOT, REDCAPTOT);
			cm->NCRET[i][j][k] = amountdef * probfactdef
				+ amountimm * probfactimm;

			amountdef = getamount(cm, k, IC, i, j, DEF, PBO,
					ART24TOT, RESTOT, REDCAPTOT);
			amountimm = getamount(cm, k, IC, i, j, IMM, PBO,
					ART24TOT, RESTOT, REDCAPTOT);
			cm->ICNCRET[i][j][k] = amountdef * probfactdef
				+ amountimm * probfactimm;
		}
	}

	for (int i = 0; i < ASSET_AMOUNT; i++) {
		amountdef = getamount(cm, k, ASSETS, 0, i, DEF, PBO,
				ART24TOT, RESTOT, REDCAPTOT);
		amountimm = getamount(cm, k, ASSETS, 0, i, IMM, PBO,
				ART24TOT, RESTOT, REDCAPTOT);
		cm->assets[i][k] = amountdef * probfactdef
			+ amountimm * probfactimm;
	}
}

void evolEBP(CurrentMember *cm, int k, 
		double ART24TOT[const static METHOD_AMOUNT],
		double RESTOT[const static METHOD_AMOUNT],
		double REDCAPTOT[const static METHOD_AMOUNT])
{
	double probfactdef = 0.0;
	double probfactimm = 0.0;
	double amountdef = 0.0;
	double amountimm = 0.0;
	double vIMM = 0.0;
	double vDEF = 0.0;
	double capdth = 0.0;
	double probs = 0.0;
	int yearIMM = 0.0;
	int yearDEF = 0.0;
	Date *Ndate = 0;
	Date *IMMdate = 0;
	Date *DEFdate = 0;

	Ndate = newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1);
	yearIMM = calcyears(cm->DOC[1], cm->DOC[k], 0);
	IMMdate = newDate(0, cm->DOC[1]->year + yearIMM, cm->DOC[1]->month, 1);
	yearDEF = max(2, 0.0, calcyears(cm->DOC[1], Ndate, 0));
	DEFdate = newDate(0, cm->DOC[1]->year + yearDEF, cm->DOC[1]->month, 1);

	if (yearIMM >= 0) {
		vIMM = pow(1 + ass.DR, -calcyears(IMMdate, cm->DOC[k], 0));
		probfactimm = (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * vIMM;

		for (int i = 0; i < METHOD_AMOUNT; i++) {
			if (PUC != i && TUC != i) continue;
			for (int j = 0; j < ASSET_AMOUNT; j++) {
				for (int l = 0; l < CF_AMOUNT; l++) {
					amountimm = getamount(cm, k, DBO, i, j,
							IMM, l, ART24TOT,
							RESTOT, REDCAPTOT);
					cm->EBP[i][j][l][yearIMM+1] +=
						amountimm * probfactimm;
				}
				amountimm = getamount(cm, k, NC, i, j, IMM,
						PBO, ART24TOT, RESTOT,
						REDCAPTOT);
				cm->PBONCCF[i][j][yearIMM+1] += amountimm
					* probfactimm;
			}
		}
		capdth = cm->CAPDTHRiskPart[k] + cm->CAPDTHRESPart[k];
		probs = cm->qx[k] * cm->kPx[k] * vIMM;
		cm->EBPDTH[TBO][yearIMM+1] += capdth * probs;
		cm->EBPDTH[PBO][yearIMM+1] += capdth * cm->FF[k] * probs;
		cm->PBODTHNCCF[yearIMM+1] += capdth * cm->FFSC[k] * probs;
	}

	vDEF = pow(1 + ass.DR, -calcyears(DEFdate, Ndate, 0));
	probfactdef = cm->wxdef[k] * cm->nPk[k] * cm->kPx[k] * vDEF;

	for (int i = 0; i < METHOD_AMOUNT; i++) {
		if (PUC != i && TUC != i) continue;
		for (int j = 0; j < ASSET_AMOUNT; j++) {
			for (int l = 0; l < CF_AMOUNT; l++) {
				amountdef = getamount(cm, k, DBO, i, j, DEF, l,
						ART24TOT, RESTOT, REDCAPTOT);
				cm->EBP[i][j][l][yearDEF+1] += amountdef
					* probfactdef;
			}
			amountdef = getamount(cm, k, NC, i, j, DEF, PBO,
					ART24TOT, RESTOT, REDCAPTOT);
			cm->PBONCCF[i][j][yearDEF+1] += amountdef
				* probfactdef;
		}
	}

	free(Ndate);
	free(IMMdate);
	free(DEFdate);
}

double getamount(const CurrentMember *cm, int k, unsigned DBONCICASS, 
		unsigned method, unsigned assets, unsigned DEFIMM, 
		unsigned PBOTBO, 
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT])
{
	switch (DBONCICASS) {
		case DBO :
			return getDBO(cm, k, method, assets, DEFIMM, PBOTBO,
					ART24TOT, RESTOT, REDCAPTOT);
		case NC :
			switch (method) {
				case PUC : return ART24TOT[PUC] * cm->FFSC[k];
				case TUC : return (ART24TOT[TUCPS_1]
							   - ART24TOT[TUC]);
				default : errExit("[%s]: method = %d\n", 
							  __func__, method);
			}
			break;
		case IC :
			switch (method) {
				case PUC : return ART24TOT[PUC] * cm->FFSC[k]
					   * ass.DR;
				case TUC : return (ART24TOT[TUCPS_1]
							   - ART24TOT[TUC])
					   * ass.DR;
				default : errExit("[%s]: method = %d\n", 
							  __func__, method);
			}
			break;
		case ASSETS : return getAssets(cm, k, assets, DEFIMM, RESTOT,
					      REDCAPTOT);
		default : errExit("[%s]: unknown amount [%d]\n", __func__,
					DBONCICASS);
	}

	return 0.0;
}

static double getDBO(const CurrentMember *cm, int k, unsigned method,
		unsigned assets, unsigned DEFIMM, unsigned PBOTBO, 
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT])
{
	double liab = 0.0;
	if (PUC == method)
		liab = ART24TOT[PUC] * pow(cm->FF[k], 1 - PBOTBO);
	else if (TUC == method)
		liab = ART24TOT[TUC];
	else
		errExit("[%s]: method = %d\n", __func__, method);

	switch (assets) {
		case PAR113 :
		case PAR115 :
			switch (DEFIMM) {
				case DEF : return max(2, liab, REDCAPTOT[TUC]);
				case IMM : return max(2, liab, RESTOT[TUC]);
				default : errExit("[%s] DEFIMM = %d\n", 
							  __func__, DEFIMM);
			}
			break;
		case MATHRES : return liab;
		default : errExit("[%s] assets = %d\n", __func__, assets);
	}
	return liab;
}

static double getAssets(const CurrentMember *cm, int k, unsigned assets,
		unsigned DEFIMM,
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT])
{
	double a = 0.0;
	double corrfactor = 1.0;

	switch (DEFIMM) {
		case DEF : a = REDCAPTOT[TUC];
			   break; 
		case IMM : a = RESTOT[TUC]; 
			   break;
		default : errExit("[%s]: DEFIMM = %d\n", __func__, DEFIMM);
	}

	if (PAR113 == assets) corrfactor = cm->vn113[k] / cm->vn[k];

	return a * corrfactor;
}

static void updateRESCAP(CurrentMember *cm, int k)
{
	CalcInput *CI = jalloc(1, sizeof(CalcInput));

	CI->RA = (k + 1 > MAXPROJBEFOREPROL ? NRA(cm, k + 1) : cm->NRA);
	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->lt = &tff.ltINS[i][gen];
			CI->res = cm->RES[PUC][i][gen][k];
			CI->prem = cm->PREMIUM[i][gen][k];
			CI->deltacap = cm->DELTACAP[i][k];
			CI->capdth = cm->CAPDTH[i][gen][k];
			CI->age = cm->age[k];
			CI->cap = calcCAP(cm, CI);

			CI->capdth = cm->CAPDTH[i][gen][k+1];
			CI->age = cm->age[k+1];

			cm->RES[PUC][i][gen][k+1] = calcRES(cm, CI);
			cm->CAP[i][gen][k] = CI->cap;
		}
	}
	free(CI);
}

static void updateRESCAPPS(CurrentMember *cm, int k)
{
	CalcInput *CI = jalloc(1, sizeof(CalcInput));
	CI->prem = 0;
	CI->deltacap = 0;
	CI->capdth = 0;

	CI->RA = (k + 1 > MAXPROJBEFOREPROL ? NRA(cm, k + 1) : cm->NRA);
	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->lt = &tff.ltINS[i][gen];
			CI->res = cm->RESPS[PUC][i][gen][k];
			CI->age = cm->age[k];
			CI->cap = calcCAP(cm, CI);

			CI->res = CI->cap;
			CI->age = cm->age[k+1];

			cm->RESPS[PUC][i][gen][k+1] = calcRES(cm, CI);
			cm->CAPPS[i][gen][k] = CI->cap;
		}
	}
	free(CI);
}

static void updateREDCAPPUC(CurrentMember *cm, int k)
{
	CalcInput *CI = jalloc(1, sizeof(CalcInput));
	CI->prem = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->RA = (k + 1 > MAXPROJBEFOREPROL ? 
					NRA(cm, k + 1) : cm->NRA);
			CI->lt = &tff.ltAfterTRM[i][gen];
			CI->res = cm->RES[PUC][i][gen][k+1];
			CI->deltacap = cm->DELTACAP[i][k+1];
			CI->capdth = cm->CAPDTH[i][gen][k+1];
			CI->age = cm->age[k+1];
			CI->cap = calcCAP(cm, CI);

			CI->lt = &tff.ltProlongAfterTRM[i];
			CI->res = CI->cap;
			CI->deltacap = 0;
			CI->age = CI->RA;
			CI->RA = NRA(cm, k + 1);
			cm->REDCAP[PUC][i][gen][k+1] = calcCAP(cm, CI);

			// Profit sharing
			CI->lt = &tff.ltAfterTRM[i][gen];
			CI->res = cm->RESPS[PUC][i][gen][k+1];
			CI->deltacap = 0;
			CI->capdth = 0;
			CI->age = cm->age[k+1];
			CI->RA = (k+1 > MAXPROJBEFOREPROL ?
					NRA(cm, k+1) : cm->NRA);
			CI->cap = calcCAP(cm, CI);

			CI->lt = &tff.ltProlongAfterTRM[i];
			CI->res = CI->cap;
			CI->age = CI->RA;
			CI->RA = NRA(cm, k + 1);
			cm->REDCAP[PUC][i][gen][k+1] += calcCAP(cm, CI);
		}
	}
	free(CI);
}

static void updateRESTUC(CurrentMember *cm, int k)
{
	double ir = 0.0;
	double Ex = 0.0;
	CalcInput *CI = jalloc(1, sizeof(CalcInput));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->RA = min(3, NRA(cm, k + 1), cm->NRA, cm->age[k+1]);
			ir = cm->TAUX[i][gen];
			Ex = nEx(tff.ltINS[i][gen].lt, ir, tff.costRES, CI->RA,
					min(2, NRA(cm, k), cm->NRA), 0);

			CI->lt = &tff.ltINS[i][gen];
			CI->res = cm->RES[PUC][i][gen][1];
			CI->capdth = cm->CAPDTH[i][gen][1];
			CI->age = cm->age[1];
			CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
				* (CI->RA - cm->age[1]) * 12 * Ex;

			CI->lt = &tff.ltProlong[i];
			CI->res = CI->cap;
			CI->age = CI->RA;
			CI->RA = cm->age[k+1];

			cm->RES[TUC][i][gen][k+1] = calcCAP(cm, CI);
			cm->RESPS[TUC][i][gen][k+1] = 
				cm->RESPS[PUC][i][gen][k+1];
		}
	}
	free(CI);
}

static void updateREDCAPTUC(CurrentMember *cm, int k)
{
	CalcInput *CI = jalloc(1, sizeof(CalcInput));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			if (cm->tariff == MIXED || cm->tariff == UKMT) {
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->capdth = cm->CAPDTH[i][gen][1];
				CI->age = cm->age[1];
				CI->RA = min(2, NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
					* (cm->NRA - CI->age) * 12;

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = cm->NRA;
				cm->REDCAP[TUC][i][gen][k+1] = calcCAP(cm, CI);

				// Profit sharing
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = cm->RESPS[PUC][i][gen][1];
				CI->capdth = 0;
				CI->age = cm->age[1];
				CI->RA = min(2, NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = cm->NRA;
				cm->REDCAP[TUC][i][gen][k+1] += calcCAP(cm, CI); 
			} else {
				CI->lt = &tff.ltINS[i][gen];
				CI->res = cm->RES[PUC][i][gen][1];
				CI->capdth = cm->CAPDTH[i][gen][1];
				CI->age = cm->age[1];
				CI->RA = min(3, NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = min(2, NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
					* (cm->NRA - cm->age[1]) * 12;

				CI->lt = &tff.ltProlong[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = max(2, min(2, NRA(cm, k + 1), cm->NRA)
						, cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = NRA(cm, k + 1);
				cm->REDCAP[TUC][i][gen][k+1] = calcCAP(cm, CI);

				// Profit sharing
				CI->lt = &tff.ltINS[i][gen];
				CI->res = cm->RESPS[PUC][i][gen][1];
				CI->capdth = 0;
				CI->age = cm->age[1];
				CI->RA = min(3, NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = min(2, NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlong[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = max(2, min(2, NRA(cm, k + 1), cm->NRA)
						, cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = NRA(cm, k + 1);
				cm->REDCAP[TUC][i][gen][k+1] += 
					calcCAP(cm, CI);
			}
		}
	}
	free(CI);
}

static void updateRESTUCPS_1(CurrentMember *cm, int k)
{
	unsigned index = min(2, (double)k+1, 2.0);
	double ir = 0.0;
	double Ex = 0.0;
	CalcInput *CI = jalloc(1, sizeof(CalcInput));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->RA = min(3, NRA(cm, k + 1), cm->NRA, cm->age[k+1]);
			ir = cm->TAUX[i][gen];
			Ex = nEx(tff.ltINS[i][gen].lt, ir, tff.costRES, CI->RA,
					min(2, NRA(cm, k), cm->NRA), 0);

			CI->lt = &tff.ltINS[i][gen];
			CI->res = cm->RES[PUC][i][gen][index];
			CI->capdth = cm->CAPDTH[i][gen][index];
			CI->age = cm->age[index];
			CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
				* (CI->RA - CI->age) * 12 * Ex;

			CI->lt = &tff.ltProlong[i];
			CI->res = CI->cap;
			CI->age = CI->RA;
			CI->RA = cm->age[k+1];

			cm->RES[TUCPS_1][i][gen][k+1] = calcCAP(cm, CI);
			cm->RESPS[TUCPS_1][i][gen][k+1] = 
				cm->RESPS[PUC][i][gen][k+1];
		}
	}
	free(CI);
}

static void updateREDCAPTUCPS_1(CurrentMember *cm, int k)
{
	unsigned index = min(2, (double)k+1, 2.0);
	CalcInput *CI = jalloc(1, sizeof(CalcInput));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			if (cm->tariff == MIXED || cm->tariff == UKMT) {
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = cm->RES[PUC][i][gen][index];
				CI->capdth = cm->CAPDTH[i][gen][index];
				CI->age = cm->age[index]; 
				CI->RA = min(2, NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
					* (cm->NRA - CI->age) * 12;

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = cm->NRA;
				cm->REDCAP[TUCPS_1][i][gen][k+1] = 
					calcCAP(cm, CI);

				// Profit sharing
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = cm->RESPS[PUC][i][gen][index];
				CI->capdth = 0;
				CI->age = cm->age[index];
				CI->RA = min(2, NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = cm->NRA;
				cm->REDCAP[TUCPS_1][i][gen][k+1] += 
					calcCAP(cm, CI); 
			} else {
				CI->lt = &tff.ltINS[i][gen];
				CI->res = cm->RES[PUC][i][gen][index];
				CI->capdth = cm->CAPDTH[i][gen][index];
				CI->age = cm->age[index];
				CI->RA = min(3, NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = min(2, NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
					* (cm->NRA - cm->age[index]) * 12;

				CI->lt = &tff.ltProlong[i];
				CI->res = CI->cap;
				CI->age = min(2, NRA(cm, k+1), cm->NRA);
				CI->RA = max(2, min(2, NRA(cm, k + 1), cm->NRA)
						, cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = NRA(cm, k + 1);
				cm->REDCAP[TUCPS_1][i][gen][k+1] = 
					calcCAP(cm, CI);

				// Profit sharing
				CI->lt = &tff.ltINS[i][gen];
				CI->res = cm->RESPS[PUC][i][gen][index];
				CI->capdth = 0;
				CI->age = cm->age[index];
				CI->RA = min(3, NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = min(2, NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlong[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = max(2, min(2, NRA(cm, k + 1), cm->NRA)
						, cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = NRA(cm, k + 1);
				cm->REDCAP[TUCPS_1][i][gen][k+1] += 
					calcCAP(cm, CI);
			}
		}
	}
	free(CI);
}
