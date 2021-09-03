#include "libraryheader.h"
#include "dates.h"
#include "actuarialfunctions.h"
#include "hashtable.h"
#include "errorexit.h"

static void updateART24ACT(CurrentMember cm[static restrict 1], int k);
static void updateART24reserves(unsigned method,
		CurrentMember cm[static restrict 1],
		unsigned EREE, unsigned ART24gen, int k);
static double get_premium_method(unsigned method,
		CurrentMember cm[static restrict 1], unsigned EREE, int k);
static void updateART24DEF(CurrentMember cm[static 1], int k);
static double getDBO(const CurrentMember cm[static restrict 1], int k,
		unsigned method, unsigned assets, unsigned DEFIMM,
		unsigned PBOTBO,
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT]);
static double getAssets(const CurrentMember cm[static restrict 1], int k,
		unsigned assets, unsigned DEFIMM,
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT]);
static void updateRESCAP(CurrentMember cm[static restrict 1], int k);
static void updateRESCAPPS(CurrentMember cm[static restrict 1], int k);
static void updateREDCAPPUC(CurrentMember cm[static restrict 1], int k);
static void updateRESTUC(CurrentMember cm[static restrict 1], int k);
static void updateREDCAPTUC(CurrentMember cm[static restrict 1], int k);
static void updateRESTUCPS_1(CurrentMember cm[static restrict 1], int k);
static void updateREDCAPTUCPS_1(CurrentMember cm[static restrict 1], int k);

/*
 * inline functions
 */
double npx(register unsigned lt, register double ageX, register double ageXn,
		register int corr) PURE ;
double nEx(register unsigned lt, register double i, register double charge,
		register double ageX, register double ageXn,
		register int corr) PURE ;
double axn(register unsigned lt, register double i, register double charge,
		register unsigned prepost, register unsigned term,
		register double ageX, register double ageXn,
		register int corr) PURE ;
double CAP_UKMS_UKZT(double res, double prem, double deltacap, double age,
		double RA, double ac, double Ex, double ax) CONST ;
double CAP_UKMT(double res, double prem, double capdth, double ac,
		double Ex, double ax, double axcost, double Ax1, double IAx1,
		double Iax, double cKO) CONST ;
double CAP_MIXED(double res, double prem, double ac, double Ex,
		double ax, double axcost, double Ax1, double x10,
		double MIXEDPS, double cKO) CONST ;

/* 
 * nAx = v^(1/2)*1Qx + v^(1+1/2)*1Px*1q_{x+1} + ...
 * + v^(n-1+1/2)*{n-1}_Px*1Q_{x+n-1}
 */
double Ax1n(register unsigned lt, register double i, register double charge,
		register double ageX, register double ageXn, register int corr)
{
	register int k = 0;
	register int payments = 0;
	register double im = 0.0;
	register double v = 0.0;
	register double value = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		im = (1 + i)/(1 + charge) - 1;
		v = 1/(1 + im);
		payments = ageXn - ageX + EPS;

		while (payments--) {
			value += pow(v, k + 1.0/2)
				* npx(lt, ageX, ageX + k, corr)
				* (1 - npx(lt, ageX + k, ageX + k + 1, corr));
			k++;
		}
		value += pow(v, k + 1.0/2)
			* npx(lt, ageX, ageX + k, corr)
			* (1 - npx(lt, ageX + k, ageXn, corr));

		return value;
	}
}

// IAx1n = sum^{n-1}_k=1:k*1A_{x+k}*kEx
double IAx1n(register unsigned lt, register double i, register double charge,
		register double ageX, register double ageXn, register int corr)
{
	register int k = 1;
	register int payments = 0;
	register double value = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		payments = ageXn - ageX + EPS;
		while (payments--) {
			value += k * Ax1n(lt, i, charge, ageX + k - 1,
					ageX + k, corr)
				* nEx(lt, i, charge, ageX, ageX + k - 1, corr);
			k++;
		}
		value += k * Ax1n(lt, i, charge, ageX + k - 1,
				ageXn, corr)
			* nEx(lt, i, charge, ageX, ageXn, corr);

		return value;
	}
}

/* 
 * This function hasn't been completed because I don't think I need it.
 * At the moment it only works for term = 1
 */
double Iaxn(register unsigned lt, register double i, register double charge,
		register unsigned prepost, register unsigned term,
		register double ageX, register double ageXn, register int corr)
{
	register int k = 1;
	register int payments = 0;
	register double ageXk = 0.0;
	register double value = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		ageXk = ageX + (double)prepost/term;
		payments = (ageXn - ageX) * term + EPS;
		while (payments--) {
			value += k++ * nEx(lt, i, charge, ageX, ageXk, corr);
			ageXk += 1.0/term;
		}
		ageXk -= 1.0/term * prepost;
		value /= term;
		value += (ageXn - ageXk) * k
			* nEx(lt, i, charge, ageX,
					((int)(ageXn*term + EPS))/term
					+ term*prepost, corr);

		return value;
	}
}

void evolCAPDTH(CurrentMember cm[static restrict 1], int k) {
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

void evolRES(CurrentMember cm[static restrict 1], int k)
{
	updateRESCAP(cm, k);
	updateRESCAPPS(cm, k);
	updateREDCAPPUC(cm, k);
	updateRESTUC(cm, k);
	updateREDCAPTUC(cm, k);
	updateRESTUCPS_1(cm, k);
	updateREDCAPTUCPS_1(cm, k);
}

void evolPremiums(CurrentMember cm[static restrict 1], int k)
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
			prem = (j < MAXGEN - 1 ? MIN2(cm->PREMIUM[i][j][k]
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

				cm->RP[i][j][k] = MAX2(0.0, (RPcap - RPres)
						* Ax1/ax)
					+ RPcap * tff.costKO * axcost;
			}			     
		}
	}

}

void evolART24(CurrentMember cm[static restrict 1], int k)
{
	if (cm->status & ACT)
		updateART24ACT(cm, k);
	else
		updateART24DEF(cm, k);
}

static void updateART24ACT(CurrentMember cm[static restrict 1], int k)
{
	for (int l = 0; l < EREE_AMOUNT; l++) {
		for (int m = 0; m < ART24GEN_AMOUNT; m++) {
			for (int j = 0; j < METHOD_AMOUNT; j++) {
				updateART24reserves(j, cm, l, m, k);
			}
		}
	}
}

static void updateART24reserves(unsigned method,
		CurrentMember cm[static restrict 1],
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
	admincost = (ER == EREE ? MIN2(ART24admincost, admincost) : 0);
	rp = gensum(cm->RP, EREE, k);
	im = pow(1 + i, 1.0/tff.term) - 1;
	premium = get_premium_method(method, cm, EREE, k);
	premium = MAX2(0.0, premium * (1 - admincost) - rp);

	cm->ART24[method][EREE][ART24gen][k+1] += premium / tff.term *
		(pow(1 + im, tff.term * period + pp) - 1 - im * pp) / im;

}
static double get_premium_method(unsigned method,
		CurrentMember cm[static restrict 1], unsigned EREE, int k)
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

static void updateART24DEF(CurrentMember cm[static restrict 1], int k)
{
	for (int j = 0; j < METHOD_AMOUNT; j++)
		for (int l = 0; l < EREE_AMOUNT; l++)
			for (int m = 0; m < ART24GEN_AMOUNT; m++)
				cm->ART24[j][l][m][k+1] = 
					cm->ART24[j][l][m][k]; 
}

double calcCAP(const CurrentMember cm[static restrict 1],
		const CalcInput *restrict CI)
{
	double res = CI->res, prem = CI->prem, deltacap = CI->deltacap, 
	       capdth = CI->capdth, age = CI->age, RA = CI->RA;
	LifeTable *lt = CI->lt;

	if (res < EPS && prem < EPS && deltacap < EPS && capdth < EPS)
		return 0.0;

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
			value = CAP_UKMS_UKZT(res, prem, deltacap, age, RA,
					tff.admincost, Ex, ax);
			break;
		case UKMT :
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
			IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
			Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);

			value = CAP_UKMT(res, prem, capdth, tff.admincost, Ex,
					ax, axcost, Ax1, IAx1, Iax,
					tff.costKO);
			break;
		case MIXED :
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);

			value = CAP_MIXED(res, prem, tff.admincost, Ex, ax,
					axcost, Ax1, cm->X10, tff.MIXEDPS,
					tff.costKO);
			break;
		default :
			die("%d is not a valid tariff.", cm->tariff);
	}

	return value;
}

double calcRES(const CurrentMember cm[static restrict 1],
		const CalcInput *restrict CI)
{
	double cap = CI->cap, prem = CI->prem, deltacap = CI->deltacap, 
	       capdth = CI->capdth, age = CI->age, RA = CI->RA;
	LifeTable *lt = CI->lt;

	if (cap < EPS && prem < EPS && deltacap < EPS && capdth < EPS)
		return 0.0;

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
			die("%d is not a valid tariff.", cm->tariff);
	}

	return value;
}

void evolDBONCIC(CurrentMember cm[static restrict 1], int k, 
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT])
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

void evolEBP(CurrentMember cm[static restrict 1], int k, 
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT])
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
	struct date *Ndate = 0;
	struct date *IMMdate = 0;
	struct date *DEFdate = 0;

	Ndate = newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1);
	yearIMM = calcyears(cm->DOC[1], cm->DOC[k], 0);
	IMMdate = newDate(0, cm->DOC[1]->year + yearIMM, cm->DOC[1]->month, 1);
	yearDEF = MAX2(0.0, calcyears(cm->DOC[1], Ndate, 0));
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

double getamount(const CurrentMember cm[static restrict 1], int k,
		unsigned DBONCICASS, unsigned method, unsigned assets,
		unsigned DEFIMM, unsigned PBOTBO, 
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
				default : die("method = %d", method);
			}
			break;
		case IC :
			switch (method) {
				case PUC : return ART24TOT[PUC] * cm->FFSC[k]
					   * ass.DR;
				case TUC : return (ART24TOT[TUCPS_1]
							   - ART24TOT[TUC])
					   * ass.DR;
				default : die("method = %d", method);
			}
			break;
		case ASSETS : return getAssets(cm, k, assets, DEFIMM, RESTOT,
					      REDCAPTOT);
		default : die("unknown amount [%d]", DBONCICASS);
	}

	return 0.0;
}

static double getDBO(const CurrentMember cm[static restrict 1], int k, unsigned method,
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
		die("method = %d", method);

	switch (assets) {
		case PAR113 :
		case PAR115 :
			switch (DEFIMM) {
				case DEF : return MAX2(liab, REDCAPTOT[TUC]);
				case IMM : return MAX2(liab, RESTOT[TUC]);
				default : die("DEFIMM = %d", DEFIMM);
			}
			break;
		case MATHRES : return liab;
		default : die("assets = %d", assets);
	}
	return liab;
}

static double getAssets(const CurrentMember cm[static restrict 1], int k,
		unsigned assets, unsigned DEFIMM,
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
		default : die("DEFIMM = %d", DEFIMM);
	}

	if (PAR113 == assets) corrfactor = cm->vn113[k] / cm->vn[k];

	return a * corrfactor;
}

static void updateRESCAP(CurrentMember cm[static restrict 1], int k)
{
	CalcInput *CI = jalloc(1, sizeof(*CI));

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

static void updateRESCAPPS(CurrentMember cm[static restrict 1], int k)
{
	CalcInput *CI = jalloc(1, sizeof(*CI));
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

static void updateREDCAPPUC(CurrentMember cm[static restrict 1], int k)
{
	CalcInput *CI = jalloc(1, sizeof(*CI));
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

static void updateRESTUC(CurrentMember cm[static restrict 1], int k)
{
	double ir = 0.0;
	double Ex = 0.0;
	CalcInput *CI = jalloc(1, sizeof(*CI));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->RA = MIN3(NRA(cm, k + 1), cm->NRA, cm->age[k+1]);
			ir = cm->TAUX[i][gen];
			Ex = nEx(tff.ltINS[i][gen].lt, ir, tff.costRES, CI->RA,
					MIN2(NRA(cm, k), cm->NRA), 0);

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

static void updateREDCAPTUC(CurrentMember cm[static restrict 1], int k)
{
	CalcInput *CI = jalloc(1, sizeof(*CI));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			if (cm->tariff == MIXED || cm->tariff == UKMT) {
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->capdth = cm->CAPDTH[i][gen][1];
				CI->age = cm->age[1];
				CI->RA = MIN2(NRA(cm, k + 1), cm->NRA);
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
				CI->RA = MIN2(NRA(cm, k + 1), cm->NRA);
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
				CI->RA = MIN3(NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = MIN2(NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
					* (cm->NRA - cm->age[1]) * 12;

				CI->lt = &tff.ltProlong[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = MAX2(MIN2(NRA(cm, k + 1), cm->NRA)
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
				CI->RA = MIN3(NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = MIN2(NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlong[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = MAX2(MIN2(NRA(cm, k + 1), cm->NRA)
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

static void updateRESTUCPS_1(CurrentMember cm[static restrict 1], int k)
{
	unsigned index = MIN2(k + 1, 2.0);
	double ir = 0.0;
	double Ex = 0.0;
	CalcInput *CI = jalloc(1, sizeof(*CI));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->RA = MIN3(NRA(cm, k + 1), cm->NRA, cm->age[k+1]);
			ir = cm->TAUX[i][gen];
			Ex = nEx(tff.ltINS[i][gen].lt, ir, tff.costRES, CI->RA,
					MIN2(NRA(cm, k), cm->NRA), 0);

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

static void updateREDCAPTUCPS_1(CurrentMember cm[static restrict 1], int k)
{
	unsigned index = MIN2(k + 1, 2.0);
	CalcInput *CI = jalloc(1, sizeof(*CI));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			if (cm->tariff == MIXED || cm->tariff == UKMT) {
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = cm->RES[PUC][i][gen][index];
				CI->capdth = cm->CAPDTH[i][gen][index];
				CI->age = cm->age[index]; 
				CI->RA = MIN2(NRA(cm, k + 1), cm->NRA);
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
				CI->RA = MIN2(NRA(cm, k + 1), cm->NRA);
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
				CI->RA = MIN3(NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = MIN2(NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
					* (cm->NRA - cm->age[index]) * 12;

				CI->lt = &tff.ltProlong[i];
				CI->res = CI->cap;
				CI->age = MIN2(NRA(cm, k+1), cm->NRA);
				CI->RA = MAX2(MIN2(NRA(cm, k + 1), cm->NRA)
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
				CI->RA = MIN3(NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = MIN2(NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlong[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = MAX2(MIN2(NRA(cm, k + 1), cm->NRA)
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
