#include "calculate.h"
#include "dates.h"
#include "assumptions.h"
#include "actuarialfunctions.h"

static double getDBO(const CurrentMember cm[restrict static 1], int k,
		unsigned method, unsigned assets, unsigned DEFIMM,
		unsigned PBOTBO,
		const double ART24TOT[const static METHOD_AMOUNT],
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT]);
static double getAssets(const CurrentMember cm[restrict static 1], int k,
		unsigned assets, unsigned DEFIMM,
		const double RESTOT[const static METHOD_AMOUNT],
		const double REDCAPTOT[const static METHOD_AMOUNT]);
static void updateRESCAP(CurrentMember cm[restrict static 1], int k);
static void updateRESCAPPS(CurrentMember cm[restrict static 1], int k);
static void updateREDCAPPUC(CurrentMember cm[restrict static 1], int k);
static void updateRESTUC(CurrentMember cm[restrict static 1], int k);
static void updateREDCAPTUC(CurrentMember cm[restrict static 1], int k);
static void updateRESTUCPS_1(CurrentMember cm[restrict static 1], int k);
static void updateREDCAPTUCPS_1(CurrentMember cm[restrict static 1], int k);

void evolCAPDTH(CurrentMember cm[restrict static 1], int k) {
	for (int i = 0; i < EREE_AMOUNT; i++) {
		cm->DELTACAP[i][k+1] = cm->DELTACAP[i][k];
		for (int gen = 0; gen < MAXGEN; gen++) {
			cm->proj[k+1].gens[gen].death_lump_sum[i]
				= cm->proj[k].gens[gen].death_lump_sum[i]
				+ cm->proj[k].gens[gen].premium[i]
				* (1 - tff.admincost)
				* (cm->age[k+1] - cm->age[k]);
		}
	}
}

void evolRES(CurrentMember cm[restrict static 1], int k)
{
	updateRESCAP(cm, k);
	updateRESCAPPS(cm, k);
	updateREDCAPPUC(cm, k);
	updateRESTUC(cm, k);
	updateREDCAPTUC(cm, k);
	updateRESTUCPS_1(cm, k);
	updateREDCAPTUCPS_1(cm, k);
}

void evolPremiums(CurrentMember cm[restrict static 1], int k)
{
	double formula = 0.0;
	double prem = 0.0;
	double Ax1 = 0.0;
	double ax = 0.0;
	double axcost = 0.0;
	double RPcap = 0.0;
	double RPres = 0.0;
	double lump_sum = 0.0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		formula = (ER == i ? calcA(cm, k + 1) : calcC(cm, k + 1));
		for (int j = 0; j < MAXGEN; j++) {
			lump_sum = cm->proj[k].gens[j].lump_sums.lump_sum[i];
			prem = formula;
			for (int l = 0; l < j; l++)
				prem = prem - cm->proj[k+1].gens[j].premium[i];
			prem = (j < MAXGEN - 1 ?
					MIN(cm->proj[k].gens[j].premium[i],
						prem) : prem);

			cm->proj[k+1].gens[j].premium[i] = prem;

			if (cm->age[k+1] == cm->age[k] 
					|| (cm->tariff != MIXED))
				cm->proj[k].gens[j].risk_premium[i] = 0;
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
				RPcap = lump_sum/cm->X10;
				RPres = cm->proj[k].gens[j].reserves.res[i].puc;

				cm->proj[k].gens[j].risk_premium[i]
					= MAX(0.0, (RPcap - RPres) * Ax1/ax)
					+ RPcap * tff.costKO * axcost;
			}			     
		}
	}

}

static double get_premium_method(size_t method, const struct generations *g,
		unsigned EREE, int k)
{
	double premium = 0.0;

	switch(method) {
		case PUC :
			premium = gen_sum(g, EREE, PREMIUM, PUC);
			break;
		case TUC :
			premium = (k > 0 ? 0 : gen_sum(g, EREE, PREMIUM, TUC));
			break;
		case TUCPS_1 :
			premium = (k > 1 ? 0 : gen_sum(g, EREE, PREMIUM,
						TUCPS_1));
			break;
		default:
			assert(0);
			break;
	}

	return premium;
}


static double get_art24_act_res(struct art24 a24_t1, double period,
		size_t EREE, size_t gen, double premium, double rp,
		size_t method)
{
	unsigned term = tff.term;
	unsigned pp = 0;
	double i = 0.0;
	double im = 0.0;
	double admincost = tff.admincost;
	double result = 0.0;
	double res = get_method_amount(a24_t1.res[EREE], method);

	i = a24_t1.i[EREE];

	result = res * pow(1 + i, period);

	if (ART24GEN1 == gen || !premium) {
		return result;
	}

	pp = (tff.prepost == 0 ? 1 : 0);
	admincost = (ER == EREE ? MIN(ART24admincost, admincost) : 0);
	im = pow(1 + i, 1.0/tff.term) - 1;
	premium = MAX(0.0, premium * (1 - admincost) - rp);

	result += premium / term
		* (pow(1 + im, term * period + pp) - 1 - im * pp) / im;
	return result;
}

#define UPDATE(m1, m2) \
	premium = get_premium_method(m1, g, i, k); \
	a24_t[j].res[i].m2 = get_art24_act_res(a24_t1[j], \
			period, i, j, premium, rp, m1);


static void update_art24_acts(struct art24 *a24_t, const struct art24 *a24_t1,
		double period, const struct generations *g, int k)
{
	double rp = 0.0;
	double premium = 0.0;
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		rp = gen_sum(g, i, RP, PUC);
		for (size_t j = 0; j < ART24GEN_AMOUNT; j++) {
			UPDATE(PUC, puc);
			UPDATE(TUC, tuc);
			UPDATE(TUCPS_1, tucps_1);
		}
	}
}

#undef UPDATE

static void update_art24_act(CurrentMember *cm, int k)
{
	assert(cm);
	double period = 0.0;
	period = cm->age[k+1] - cm->age[k];
	update_art24_acts(cm->proj[k+1].art24, cm->proj[k].art24,
			period, cm->proj[k].gens, k);
}

static void update_art24_def(struct art24 *a24_t, const struct art24 *a24_t1)
{
	assert(a24_t);
	assert(a24_t1);
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		for (size_t j = 0; j < ART24GEN_AMOUNT; j++) {
			a24_t[j].res[i].puc = a24_t1[j].res[i].puc;
			a24_t[j].res[i].tuc = a24_t1[j].res[i].tuc;
			a24_t[j].res[i].tucps_1 = a24_t1[j].res[i].tucps_1;
		}
	}
}

void update_art24(CurrentMember cm[restrict static 1], int k)
{
	assert(cm);
	if (cm->status & ACT) {
		update_art24_act(cm, k);
	} else {
		update_art24_def(cm->proj[k+1].art24, cm->proj[k].art24);
	}
}

double calcCAP(const CurrentMember cm[restrict static 1],
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

double calcRES(const CurrentMember cm[restrict static 1],
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

void evolDBONCIC(CurrentMember cm[restrict static 1], int k, 
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

void evolEBP(CurrentMember cm[restrict static 1], int k, 
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

double getamount(const CurrentMember cm[restrict static 1], int k,
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

static double getDBO(const CurrentMember cm[restrict static 1], int k, unsigned method,
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

static double getAssets(const CurrentMember cm[restrict static 1], int k,
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

static void updateRESCAP(CurrentMember cm[restrict static 1], int k)
{
	CalcInput *CI = jalloc(1, sizeof(*CI));

	CI->RA = (k + 1 > MAXPROJBEFOREPROL ? NRA(cm, k + 1) : cm->NRA);
	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->lt = &tff.ltINS[i][gen];
			CI->res = cm->proj[k].gens[gen].reserves.res[i].puc;
			CI->prem = cm->proj[k].gens[gen].premium[i];
			CI->deltacap = cm->DELTACAP[i][k];
			CI->capdth = cm->proj[k].gens[gen].death_lump_sum[i];
			CI->age = cm->age[k];
			CI->cap = calcCAP(cm, CI);

			CI->capdth = cm->proj[k+1].gens[gen].death_lump_sum[i];
			CI->age = cm->age[k+1];

			cm->proj[k+1].gens[gen].reserves.res[i].puc
				= calcRES(cm, CI);
			cm->proj[k].gens[gen].lump_sums.lump_sum[i] = CI->cap;
		}
	}
	free(CI);
}

static void updateRESCAPPS(CurrentMember cm[restrict static 1], int k)
{
	CalcInput *CI = jalloc(1, sizeof(*CI));
	CI->prem = 0;
	CI->deltacap = 0;
	CI->capdth = 0;

	CI->RA = (k + 1 > MAXPROJBEFOREPROL ? NRA(cm, k + 1) : cm->NRA);
	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->lt = &tff.ltINS[i][gen];
			CI->res = cm->proj[k].gens[gen].reserves.ps[i];
			CI->age = cm->age[k];
			CI->cap = calcCAP(cm, CI);

			CI->res = CI->cap;
			CI->age = cm->age[k+1];

			cm->proj[k+1].gens[gen].reserves.ps[i]
				= calcRES(cm, CI);
			cm->proj[k].gens[gen].lump_sums.ps[i] = CI->cap;
		}
	}
	free(CI);
}

static void updateREDCAPPUC(CurrentMember cm[restrict static 1], int k)
{
	CalcInput *CI = jalloc(1, sizeof(*CI));
	CI->prem = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			CI->RA = (k + 1 > MAXPROJBEFOREPROL ? 
					NRA(cm, k + 1) : cm->NRA);
			CI->lt = &tff.ltAfterTRM[i][gen];
			CI->res = cm->proj[k+1].gens[gen].reserves.res[i].puc;
			CI->deltacap = cm->DELTACAP[i][k+1];
			CI->capdth = cm->proj[k+1].gens[gen].death_lump_sum[i];
			CI->age = cm->age[k+1];
			CI->cap = calcCAP(cm, CI);

			CI->lt = &tff.ltProlongAfterTRM[i];
			CI->res = CI->cap;
			CI->deltacap = 0;
			CI->age = CI->RA;
			CI->RA = NRA(cm, k + 1);
			cm->proj[k+1].gens[gen].lump_sums.reduced[i].puc =
				calcCAP(cm, CI);

			// Profit sharing
			CI->lt = &tff.ltAfterTRM[i][gen];
			CI->res = cm->proj[k+1].gens[gen].reserves.ps[i];
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
			cm->proj[k+1].gens[gen].lump_sums.reduced[i].puc +=
				calcCAP(cm, CI);
		}
	}
	free(CI);
}

static void updateRESTUC(CurrentMember cm[restrict static 1], int k)
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
			CI->res = cm->proj[1].gens[gen].reserves.res[i].puc;
			CI->capdth = cm->proj[1].gens[gen].death_lump_sum[i];
			CI->age = cm->age[1];
			CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
				* (CI->RA - cm->age[1]) * 12 * Ex;

			CI->lt = &tff.ltProlong[i];
			CI->res = CI->cap;
			CI->age = CI->RA;
			CI->RA = cm->age[k+1];

			cm->proj[k+1].gens[gen].reserves.res[i].tuc
				= calcCAP(cm, CI);
		}
	}
	free(CI);
}

static void updateREDCAPTUC(CurrentMember cm[restrict static 1], int k)
{
	CalcInput *CI = jalloc(1, sizeof(*CI));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			if (cm->tariff == MIXED || cm->tariff == UKMT) {
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->capdth
					= cm->proj[1].gens[gen].death_lump_sum[i];
				CI->age = cm->age[1];
				CI->RA = MIN(NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
					* (cm->NRA - CI->age) * 12;

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = cm->NRA;
				cm->proj[k+1].gens[gen].lump_sums.reduced[i].tuc =
					calcCAP(cm, CI);

				// Profit sharing
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = cm->proj[1].gens[gen].reserves.ps[i];
				CI->capdth = 0;
				CI->age = cm->age[1];
				CI->RA = MIN(NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = cm->NRA;
				cm->proj[k+1].gens[gen].lump_sums.reduced[i].tuc +=
					calcCAP(cm, CI);
			} else {
				CI->lt = &tff.ltINS[i][gen];
				CI->res =
					cm->proj[1].gens[gen].reserves.res[i].puc;
				CI->capdth
					= cm->proj[1].gens[gen].death_lump_sum[i];
				CI->age = cm->age[1];
				CI->RA = MIN3(NRA(cm, k + 1), cm->NRA, 
						cm->age[k+1]);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = MIN(NRA(cm, k + 1), cm->NRA);
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
				cm->proj[k+1].gens[gen].lump_sums.reduced[i].tuc =
					calcCAP(cm, CI);

				// Profit sharing
				CI->lt = &tff.ltINS[i][gen];
				CI->res = cm->proj[1].gens[gen].reserves.ps[i];
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
				cm->proj[k+1].gens[gen].lump_sums.reduced[i].tuc +=
					calcCAP(cm, CI);
			}
		}
	}
	free(CI);
}

static void updateRESTUCPS_1(CurrentMember cm[restrict static 1], int k)
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
			CI->res = cm->proj[index].gens[gen].reserves.res[i].puc;
			CI->capdth = cm->proj[index].gens[gen].death_lump_sum[i];
			CI->age = cm->age[index];
			CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
				* (CI->RA - CI->age) * 12 * Ex;

			CI->lt = &tff.ltProlong[i];
			CI->res = CI->cap;
			CI->age = CI->RA;
			CI->RA = cm->age[k+1];

			cm->proj[k+1].gens[gen].reserves.res[i].tucps_1
				= calcCAP(cm, CI);
		}
	}
	free(CI);
}

static void updateREDCAPTUCPS_1(CurrentMember cm[restrict static 1], int k)
{
	unsigned index = MIN(k + 1, 2.0);
	CalcInput *CI = jalloc(1, sizeof(*CI));
	CI->prem = 0;
	CI->deltacap = 0;

	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			if (cm->tariff == MIXED || cm->tariff == UKMT) {
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res =
					cm->proj[index].gens[gen].reserves.res[i].puc;
				CI->capdth =
					cm->proj[index].gens[gen].death_lump_sum[i];
				CI->age = cm->age[index]; 
				CI->RA = MIN(NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI) + cm->DELTACAP[i][0]
					* (cm->NRA - CI->age) * 12;

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = cm->NRA;
				cm->proj[k+1].gens[gen].lump_sums.reduced[i].tucps_1 =
					calcCAP(cm, CI);

				// Profit sharing
				CI->lt = &tff.ltAfterTRM[i][gen];
				CI->res = cm->proj[index].gens[gen].reserves.ps[i];
				CI->capdth = 0;
				CI->age = cm->age[index];
				CI->RA = MIN(NRA(cm, k + 1), cm->NRA);
				CI->cap = calcCAP(cm, CI);

				CI->lt = &tff.ltProlongAfterTRM[i];
				CI->res = CI->cap;
				CI->age = CI->RA;
				CI->RA = cm->NRA;
				cm->proj[k+1].gens[gen].lump_sums.reduced[i].tucps_1
					+= calcCAP(cm, CI);
			} else {
				CI->lt = &tff.ltINS[i][gen];
				CI->res =
					cm->proj[index].gens[gen].reserves.res[i].puc;
				CI->capdth =
					cm->proj[index].gens[gen].death_lump_sum[i];
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
				cm->proj[k+1].gens[gen].lump_sums.reduced[i].tucps_1
					= calcCAP(cm, CI);

				// Profit sharing
				CI->lt = &tff.ltINS[i][gen];
				CI->res = cm->proj[index].gens[gen].reserves.ps[i];
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
				cm->proj[k+1].gens[gen].lump_sums.reduced[i].tucps_1
					+= calcCAP(cm, CI);
			}
		}
	}
	free(CI);
}
