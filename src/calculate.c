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
double calc_lump_sum(unsigned tariff, double X10,
		double res, double prem, double deltacap, double capdth,
		double age, double RA, LifeTable *lt);
double calc_res(unsigned tariff, double X10, double cap, double prem,
		double deltacap, double capdth, double age, double RA,
		LifeTable *lt);

void evolCAPDTH(CurrentMember cm[restrict static 1], int k) {
	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			cm->proj[k+1].gens[gen].death_lump_sum[i]
				= cm->proj[k].gens[gen].death_lump_sum[i]
				+ cm->proj[k].gens[gen].premium[i]
				* (1 - tff.admincost)
				* (cm->age[k+1] - cm->age[k]);
		}
	}
}

static double update_res(double *target, unsigned tariff, double X10,
		double res, double prem, double deltacap, double capdth,
		double age_k, double age_k1, double RA, LifeTable *lt)
{
	assert(target);
	double cap = calc_lump_sum(tariff, X10, res, prem, deltacap, capdth,
			age_k, RA, lt);
	*target = calc_res(tariff, X10, cap, prem, deltacap, capdth, age_k1,
			RA, lt);
	return cap;
}

static double update_res_puc(CurrentMember *cm, size_t EREE, size_t gen,
		size_t k)
{
	LifeTable *lt = &tff.ltINS[EREE][gen];
	double *target = &cm->proj[k+1].gens[gen].reserves.res[EREE].puc;
	double RA = (k + 1 > MAXPROJBEFOREPROL ? NRA(cm, k + 1) : cm->NRA);
	double capdth = cm->proj[k].gens[gen].death_lump_sum[EREE];
	double delta_cap = cm->proj[k].delta_cap[EREE];
	double prem = cm->proj[k].gens[gen].premium[EREE];
	double res = cm->proj[k].gens[gen].reserves.res[EREE].puc;
	double cap = update_res(target, cm->tariff, cm->X10, res, prem,
			delta_cap, capdth, cm->age[k], cm->age[k+1], RA, lt);
	return cap;
}

static double update_res_ps(CurrentMember *cm, size_t EREE, size_t gen,
		size_t k)
{
	LifeTable *lt = &tff.ltINS[EREE][gen];
	double *target = &cm->proj[k+1].gens[gen].reserves.ps[EREE];
	double RA = (k + 1 > MAXPROJBEFOREPROL ? NRA(cm, k + 1) : cm->NRA);
	double res = cm->proj[k].gens[gen].reserves.ps[EREE];
	double cap = update_res(target, cm->tariff, cm->X10, res,
			0.0, 0.0, 0.0, cm->age[k], cm->age[k+1], RA, lt);
	return cap;
}

static double get_res_tuc(const CurrentMember *cm, size_t EREE, size_t gen,
		size_t k, size_t index)
{
	LifeTable *lt = &tff.ltINS[EREE][gen];
	double i = cm->TAUX[EREE][gen];
	double RA = MIN3(NRA(cm, k + 1), cm->NRA, cm->age[k+1]);
	double Ex = nEx(tff.ltINS[EREE][gen].lt, i, tff.costRES, RA,
					MIN(NRA(cm, k), cm->NRA), 0);
	double capdth = cm->proj[index].gens[gen].death_lump_sum[EREE];
	double res = cm->proj[index].gens[gen].reserves.res[EREE].puc;
	double cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth,
			cm->age[index], RA, lt);
	cap += cm->proj[0].delta_cap[EREE] * (RA - cm->age[index]) * 12 * Ex;
	lt = &tff.ltProlong[EREE];
	res = calc_lump_sum(cm->tariff, cm->X10, cap, 0.0, 0.0, capdth, RA,
			cm->age[k+1], lt);
	return res;
}

static void update_res_tucs(CurrentMember *cm, size_t EREE, size_t gen,
		size_t k)
{
	cm->proj[k+1].gens[gen].reserves.res[EREE].tuc
		= get_res_tuc(cm, EREE, gen, k, 1);
	cm->proj[k+1].gens[gen].reserves.res[EREE].tucps_1
		= get_res_tuc(cm, EREE, gen, k, MIN(k+1, 2));
}

static double get_redcap_puc(CurrentMember *cm, size_t EREE, size_t gen,
		size_t k)
{
	LifeTable *lt = &tff.ltAfterTRM[EREE][gen];
	double RA = (k + 1 > MAXPROJBEFOREPROL ? NRA(cm, k + 1) : cm->NRA);
	double capdth = cm->proj[k+1].gens[gen].death_lump_sum[EREE];
	double delta_cap = cm->proj[k+1].delta_cap[EREE];
	double res = cm->proj[k+1].gens[gen].reserves.res[EREE].puc;
	double age = cm->age[k+1];
	double cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, delta_cap,
			capdth, age, RA, lt);

	lt = &tff.ltProlongAfterTRM[EREE];
	res = cap;
	age = RA;
	RA = NRA(cm, k+1);
	cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth, age,
			RA, lt);

	lt = &tff.ltAfterTRM[EREE][gen];
	res = cm->proj[k+1].gens[gen].reserves.ps[EREE];
	age = cm->age[k+1];
	RA = (k + 1 > MAXPROJBEFOREPROL ? NRA(cm, k + 1) : cm->NRA);
	double capps = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, 0.0,
			age, RA, lt);

	lt = &tff.ltProlongAfterTRM[EREE];
	res = capps;
	age = RA;
	RA = NRA(cm, k+1);
	capps = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, 0.0,
			age, RA, lt);
	return cap + capps;
}
static double get_redcap_mixed_ukmt(const CurrentMember *cm, size_t EREE, size_t gen,
		size_t k, size_t index)
{
	LifeTable *lt = &tff.ltAfterTRM[EREE][gen];
	double RA = MIN(NRA(cm, k + 1), cm->NRA);
	double capdth = cm->proj[index].gens[gen].death_lump_sum[EREE];
	double res = cm->proj[index].gens[gen].reserves.res[EREE].puc;
	double age = cm->age[index];
	double cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth,
			age, RA, lt)
		+ cm->proj[0].delta_cap[EREE] * (cm->NRA - age) * 12;

	lt = &tff.ltProlongAfterTRM[EREE];
	res = cap;
	age = RA;
	RA = cm->NRA;
	cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth,
			age, RA, lt);

	lt = &tff.ltAfterTRM[EREE][gen];
	res = cm->proj[index].gens[gen].reserves.ps[EREE];
	age = cm->age[index];
	RA = MIN(NRA(cm, k + 1), cm->NRA);
	double capps = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, 0.0,
			age, RA, lt);

	lt = &tff.ltProlongAfterTRM[EREE];
	res = capps;
	age = RA;
	RA = cm->NRA;
	capps = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, 0.0,
			age, RA, lt);
	return cap + capps;
}

static double get_redcap_ukms_ukzt(const CurrentMember *cm, size_t EREE,
		size_t gen, size_t k, size_t index)
{
	LifeTable *lt = &tff.ltINS[EREE][gen];
	double RA = MIN3(NRA(cm, k + 1), cm->NRA, cm->age[k+1]);
	double capdth = cm->proj[index].gens[gen].death_lump_sum[EREE];
	double res = cm->proj[index].gens[gen].reserves.res[EREE].puc;
	double age = cm->age[index];
	double cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth,
			age, RA, lt);

	lt = &tff.ltAfterTRM[EREE][gen];
	res = cap;
	age = RA;
	RA = MIN(NRA(cm, k + 1), cm->NRA);
	cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth,
			age, RA, lt)
		+ cm->proj[0].delta_cap[EREE] * (cm->NRA - age) * 12;

	lt = &tff.ltProlong[EREE];
	res = cap;
	age = RA;
	RA = MAX(MIN(NRA(cm, k+1), cm->NRA), cm->age[k+1]);
	cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth,
			age, RA, lt);
	
	lt = &tff.ltProlongAfterTRM[EREE];
	res = cap;
	age = RA;
	RA = NRA(cm, k+1);
	cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth,
			age, RA, lt);

	lt = &tff.ltINS[EREE][gen];
	res = cm->proj[index].gens[gen].reserves.ps[EREE];
	age = cm->age[index];
	RA = MIN3(NRA(cm, k + 1), cm->NRA, cm->age[k+1]);
	double capps = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, 0.0,
			age, RA, lt);

	lt = &tff.ltAfterTRM[EREE][gen];
	res = capps;
	age = RA;
	RA = MIN(NRA(cm, k + 1), cm->NRA);
	capps = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, 0.0,
			age, RA, lt);

	lt = &tff.ltProlong[EREE];
	res = capps;
	age = RA;
	RA = MAX(MIN(NRA(cm, k+1), cm->NRA), cm->age[k+1]);
	capps = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, 0.0,
			age, RA, lt);

	lt = &tff.ltProlongAfterTRM[EREE];
	res = capps;
	age = RA;
	RA = NRA(cm, k+1);
	capps = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, 0.0,
			age, RA, lt);
	return cap + capps;
}

static double get_redcap_tuc(const CurrentMember *cm, size_t EREE, size_t gen,
		size_t k, size_t index)
{
	if (MIXED == cm->tariff || UKMT == cm->tariff) {
		return get_redcap_mixed_ukmt(cm, EREE, gen, k, index);
	} else {
		return get_redcap_ukms_ukzt(cm, EREE, gen, k, index);
	}
}

static void update_redcaps(CurrentMember *cm, size_t EREE, size_t gen,
		size_t k)
{
	cm->proj[k+1].gens[gen].lump_sums.reduced[EREE].puc
		= get_redcap_puc(cm, EREE, gen, k);
	cm->proj[k+1].gens[gen].lump_sums.reduced[EREE].tuc
		= get_redcap_tuc(cm, EREE, gen, k, 1);
	cm->proj[k+1].gens[gen].lump_sums.reduced[EREE].tucps_1
		= get_redcap_tuc(cm, EREE, gen, k, MIN(k+1, 2));
}

static void update_res_and_lump_sum(CurrentMember *cm, size_t EREE, size_t gen,
		size_t k)
{
	double cap = update_res_puc(cm, EREE, gen, k);
	cm->proj[k].gens[gen].lump_sums.lump_sum[EREE] = cap;
	cap = update_res_ps(cm, EREE, gen, k);
	cm->proj[k].gens[gen].lump_sums.ps[EREE] = cap;
	update_res_tucs(cm, EREE, gen, k);
	update_redcaps(cm, EREE, gen, k);
}

void evolRES(CurrentMember cm[restrict static 1], int k)
{
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		for (size_t j = 0; j < MAXGEN; j++) {
			update_res_and_lump_sum(cm, i, j, k);
		}
	}
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

static void check_art24_def(const struct art24 *a24_t,
		const struct art24 *a24_t1)
{
	assert(a24_t);
	assert(a24_t1);
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		for (size_t j = 0; j < ART24GEN_AMOUNT; j++) {
			assert(a24_t[j].res[i].puc == a24_t1[j].res[i].puc);
			assert(a24_t[j].res[i].tuc == a24_t1[j].res[i].tuc);
			assert(a24_t[j].res[i].tucps_1
					== a24_t1[j].res[i].tucps_1);
		}
	}
}

void update_art24(CurrentMember cm[restrict static 1], int k)
{
	assert(cm);
	if (cm->status & ACT) {
		update_art24_act(cm, k);
	} else {
		check_art24_def(cm->proj[k+1].art24, cm->proj[k].art24);
	}
}

double calc_lump_sum(unsigned tariff, double X10,
		double res, double prem, double deltacap, double capdth,
		double age, double RA, LifeTable *lt)
{
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

	switch(tariff) {
		case UKMS:
		case UKZT:
			value = CAP_UKMS_UKZT(res, prem, deltacap, age, RA,
					tff.admincost, Ex, ax);
			break;
		case UKMT:
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
			IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
			Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);

			value = CAP_UKMT(res, prem, capdth, tff.admincost, Ex,
					ax, axcost, Ax1, IAx1, Iax,
					tff.costKO);
			break;
		case MIXED:
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);

			value = CAP_MIXED(res, prem, tff.admincost, Ex, ax,
					axcost, Ax1, X10, tff.MIXEDPS,
					tff.costKO);
			break;
		default :
			die("%d is not a valid tariff.", tariff);
	}

	return value;
}

double calc_res(unsigned tariff, double X10, double cap, double prem,
		double deltacap, double capdth, double age, double RA,
		LifeTable *lt)
{
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

	switch(tariff) {
		case UKMS:
		case UKZT:
			value = (cap - deltacap * (RA - age) * 12) * Ex
				- prem * (1 - tff.admincost) * ax;
			break;
		case UKMT:
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);
			IAx1 = IAx1n(lt->lt, i, tff.costRES, age, RA, 0);
			Iax = Iaxn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);

			value = cap * Ex - prem * (1 - tff.admincost) * ax
				+ capdth * (Ax1 + tff.costKO * axcost)
				+ prem * (1 - tff.admincost)
				* (IAx1 + tff.costKO * Iax);
			break;
		case MIXED:
			axcost = axn(lt->lt, i, tff.costRES, 0, 1, age, RA, 0);
			Ax1 = Ax1n(lt->lt, i, tff.costRES, age, RA, 0);

			value = cap * (Ex + 1.0/X10 * tff.MIXEDPS
					* (Ax1 + tff.costKO * axcost))
				- prem * (1 - tff.admincost) * ax;
			break;
		default:
			die("%d is not a valid tariff.", tariff);
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

	probfactdef = cm->proj[k].factor.wxdef * cm->proj[k].factor.kPx
		* cm->proj[k].factor.nPk * cm->proj[k].factor.vn;
	probfactimm = (cm->proj[k].factor.wximm + cm->proj[k].factor.retx)
		* cm->proj[k].factor.kPx * cm->proj[k].factor.vk;

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
	yearDEF = MAX(0.0, calcyears(cm->DOC[1], Ndate, 0));
	DEFdate = newDate(0, cm->DOC[1]->year + yearDEF, cm->DOC[1]->month, 1);

	if (yearIMM >= 0) {
		vIMM = pow(1 + ass.DR, -calcyears(IMMdate, cm->DOC[k], 0));
		probfactimm = (cm->proj[k].factor.wximm + cm->proj[k].factor.retx)
			* cm->proj[k].factor.kPx * vIMM;

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
		capdth = cm->proj[k].death_risk + cm->proj[k].death_res;
		probs = cm->proj[k].factor.qx * cm->proj[k].factor.kPx * vIMM;
		cm->EBPDTH[TBO][yearIMM+1] += capdth * probs;
		cm->EBPDTH[PBO][yearIMM+1] += capdth * cm->proj[k].factor.ff * probs;
		cm->PBODTHNCCF[yearIMM+1] += capdth * cm->proj[k].factor.ff_sc * probs;
	}

	vDEF = pow(1 + ass.DR, -calcyears(DEFdate, Ndate, 0));
	probfactdef = cm->proj[k].factor.wxdef * cm->proj[k].factor.nPk
		* cm->proj[k].factor.kPx * vDEF;

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
		case DBO:
			return getDBO(cm, k, method, assets, DEFIMM, PBOTBO,
					ART24TOT, RESTOT, REDCAPTOT);
		case NC:
			switch (method) {
				case PUC: return ART24TOT[PUC] *
					   cm->proj[k].factor.ff_sc;
				case TUC: return (ART24TOT[TUCPS_1]
							   - ART24TOT[TUC]);
				default: die("method = %d", method);
			}
			break;
		case IC:
			switch (method) {
				case PUC: return ART24TOT[PUC] *
					   cm->proj[k].factor.ff_sc * ass.DR;
				case TUC: return (ART24TOT[TUCPS_1]
							   - ART24TOT[TUC])
					   * ass.DR;
				default: die("method = %d", method);
			}
			break;
		case ASSETS: return getAssets(cm, k, assets, DEFIMM, RESTOT,
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
		liab = ART24TOT[PUC] * pow(cm->proj[k].factor.ff, 1 - PBOTBO);
	else if (TUC == method)
		liab = ART24TOT[TUC];
	else
		die("method = %d", method);

	switch (assets) {
		case PAR113 :
		case PAR115 :
			switch (DEFIMM) {
				case DEF: return MAX(liab, REDCAPTOT[TUC]);
				case IMM: return MAX(liab, RESTOT[TUC]);
				default: die("DEFIMM = %d", DEFIMM);
			}
			break;
		case MATHRES : return liab;
		default: die("assets = %d", assets);
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
		case DEF: 
			a = REDCAPTOT[TUC];
			break; 
		case IMM: 
			a = RESTOT[TUC]; 
			break;
		default:
			die("DEFIMM = %d", DEFIMM);
	}

	if (PAR113 == assets) {
		corrfactor = cm->proj[k].factor.vn113 / cm->proj[k].factor.vn;
	}

	return a * corrfactor;
}
