#include "calculate.h"
#include "dates.h"
#include "assumptions.h"
#include "actuarialfunctions.h"

double calc_lump_sum(unsigned tariff, double X10,
		double res, double prem, double deltacap, double capdth,
		double age, double RA, LifeTable *lt);
double calc_res(unsigned tariff, double X10, double cap, double prem,
		double deltacap, double capdth, double age, double RA,
		LifeTable *lt);

static double get_death_lump_sum(const CurrentMember *cm, int k, size_t EREE,
size_t gen)
{
	double ls = cm->proj[k].gens[gen].lump_sums.death_lump_sum[EREE];
	double premium = cm->proj[k].gens[gen].premium[EREE];
	double ac = tff.admincost;
	double age_k1 = cm->proj[k+1].age;
	double age_k = cm->proj[k].age;

	return ls + premium * (1 - ac) * (age_k1 - age_k);
}

void update_death_lump_sums(CurrentMember cm[static 1], int k) {
	assert(cm);
	for (int i = 0; i < EREE_AMOUNT; i++) {
		for (int gen = 0; gen < MAXGEN; gen++) {
			cm->proj[k+1].gens[gen].lump_sums.death_lump_sum[i]
				= get_death_lump_sum(cm, k, i, gen);
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
	double capdth = cm->proj[k].gens[gen].lump_sums.death_lump_sum[EREE];
	double delta_cap = cm->proj[k].delta_cap[EREE];
	double prem = cm->proj[k].gens[gen].premium[EREE];
	double res = cm->proj[k].gens[gen].reserves.res[EREE].puc;
	double cap = update_res(target, cm->tariff, cm->X10, res, prem,
			delta_cap, capdth, cm->proj[k].age, cm->proj[k+1].age, RA, lt);
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
			0.0, 0.0, 0.0, cm->proj[k].age, cm->proj[k+1].age, RA, lt);
	return cap;
}

static double get_res_tuc(const CurrentMember *cm, size_t EREE, size_t gen,
		size_t k, size_t index)
{
	LifeTable *lt = &tff.ltINS[EREE][gen];
	double i = cm->TAUX[EREE][gen];
	double RA = MIN3(NRA(cm, k + 1), cm->NRA, cm->proj[k+1].age);
	double Ex = nEx(tff.ltINS[EREE][gen].lt, i, tff.costRES, RA,
			MIN(NRA(cm, k), cm->NRA), 0);
	double capdth = cm->proj[index].gens[gen].lump_sums.death_lump_sum[EREE];
	double res = cm->proj[index].gens[gen].reserves.res[EREE].puc;
	double cap = calc_lump_sum(cm->tariff, cm->X10, res, 0.0, 0.0, capdth,
			cm->proj[index].age, RA, lt);
	cap += cm->proj[0].delta_cap[EREE] * (RA - cm->proj[index].age) * 12 * Ex;
	lt = &tff.ltProlong[EREE];
	res = calc_lump_sum(cm->tariff, cm->X10, cap, 0.0, 0.0, capdth, RA,
			cm->proj[k+1].age, lt);
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
	double capdth = cm->proj[k+1].gens[gen].lump_sums.death_lump_sum[EREE];
	double delta_cap = cm->proj[k+1].delta_cap[EREE];
	double res = cm->proj[k+1].gens[gen].reserves.res[EREE].puc;
	double age = cm->proj[k+1].age;
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
	age = cm->proj[k+1].age;
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
	double capdth = cm->proj[index].gens[gen].lump_sums.death_lump_sum[EREE];
	double res = cm->proj[index].gens[gen].reserves.res[EREE].puc;
	double age = cm->proj[index].age;
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
	age = cm->proj[index].age;
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
	double RA = MIN3(NRA(cm, k + 1), cm->NRA, cm->proj[k+1].age);
	double capdth = cm->proj[index].gens[gen].lump_sums.death_lump_sum[EREE];
	double res = cm->proj[index].gens[gen].reserves.res[EREE].puc;
	double age = cm->proj[index].age;
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
	RA = MAX(MIN(NRA(cm, k+1), cm->NRA), cm->proj[k+1].age);
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
	age = cm->proj[index].age;
	RA = MIN3(NRA(cm, k + 1), cm->NRA, cm->proj[k+1].age);
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
	RA = MAX(MIN(NRA(cm, k+1), cm->NRA), cm->proj[k+1].age);
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

void update_res_lump_sums(CurrentMember cm[static 1], int k)
{
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		for (size_t j = 0; j < MAXGEN; j++) {
			update_res_and_lump_sum(cm, i, j, k);
		}
	}
}

static double get_premium(const CurrentMember *cm,
		int k, size_t EREE, size_t gen)
{
	double prem = (ER == EREE ? calcA(cm, k + 1) : calcC(cm, k + 1));
	double prem_prev = cm->proj[k].gens[gen].premium[EREE];
	for (size_t l = 0; l < gen; l++) {
		prem = prem - cm->proj[k+1].gens[gen].premium[EREE];
	}
	prem = (gen < MAXGEN - 1 ?  MIN(prem_prev, prem) : prem);

	return prem;
}

static double get_risk_premium(const CurrentMember *cm,
		int k, size_t EREE, size_t gen)
{
	double age_k1 = cm->proj[k+1].age;
	double age_k = cm->proj[k].age;

	if (age_k1 == age_k || (MIXED != cm->tariff)) {
		return 0.0;
	} 

	unsigned lt = tff.ltINS[EREE][gen].lt;
	double cR = tff.costRES;
	double cK = tff.costKO;
	double ir = cm->TAUX[EREE][gen];
	double Ax1 = Ax1n(lt, ir, cR, age_k, age_k1, 0);
	double ax = axn(lt, ir, cR, tff.prepost, tff.term, age_k, age_k1, 0);
	double axcost = axn(lt, ir, cR, 0, 1, age_k, age_k1, 0);
	double lump_sum = cm->proj[k].gens[gen].lump_sums.lump_sum[EREE];
	double RPcap = lump_sum/cm->X10;
	double RPres = cm->proj[k].gens[gen].reserves.res[EREE].puc;

	return MAX(0.0, (RPcap - RPres) * Ax1/ax) + RPcap * cK *axcost;
}

void update_premiums(CurrentMember cm[static 1], int k)
{
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		for (size_t j = 0; j < MAXGEN; j++) {
			cm->proj[k+1].gens[j].premium[i]
				= get_premium(cm, k, i, j);
			cm->proj[k].gens[j].risk_premium[i]
				= get_risk_premium(cm, k, i, j);
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
	period = cm->proj[k+1].age - cm->proj[k].age;
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

static double get_assets(unsigned assets, unsigned def_imm, double res,
		double red_cap, struct factor f)
{
	double a = 0.0;
	double corrfactor = 0.0;

	switch (def_imm) {
		case DEF: 
			a = red_cap;
			break; 
		case IMM: 
			a = res;
			break;
		default:
			die("should be deferred or immediate, "
					"got something unknown");
	}

	switch (assets) {
		case PAR115:
		case MATHRES:
			corrfactor = 1.0;
			break;
		case PAR113:
			corrfactor = f.vn113 / f.vn;
			break;
		default:
			assert(0);
			break;
	}
	return a * corrfactor;
}

static double get_dbo(unsigned method, unsigned ASSETS,
		unsigned def_imm, unsigned PBOTBO, 
		const double art24[const static METHOD_AMOUNT],
		double res, double red_cap, struct factor f)
{
	double liab = 0.0;
	double assets = get_assets(ASSETS, def_imm, res, red_cap, f);
	switch (method) {
		case PUC:
			liab = art24[PUC] * pow(f.ff, 1 - PBOTBO);
			break;
		case TUC:
			liab = art24[TUC];
			break;
		default:
			die("Unknown method");
	}

	switch (ASSETS) {
		case PAR113 :
		case PAR115 :
			return MAX(liab, assets);
			break;
		case MATHRES : return liab;
		default: die("unknown assets = %d", ASSETS);
	}
	return liab;
}

static double get_nc(unsigned method, struct factor f,
		const double art24[const static METHOD_AMOUNT])
{
	double ff = f.ff_sc;
	switch (method) {
		case PUC:
			return art24[PUC] * ff;
		case TUC:
			return (art24[TUCPS_1] - art24[TUC]);
		default:
			die("method = %d", method);
			return 0.0;
	}
}

#define DBO(m1, m2, a) \
	amountdef = get_dbo(m1, a, DEF, PBO, art24, res, red_cap, f); \
	amountimm = get_dbo(m1, a, IMM, PBO, art24, res, red_cap, f); \
	dbo->m2[a] = amountdef * probfactdef + amountimm * probfactimm;
#define NC(m1, m2, a) \
	amountdef = get_nc(m1, f, art24); \
	amountimm = amountdef; \
	nc->m2[a] = amountdef * probfactdef + amountimm * probfactimm;
#define IC_NC(m1, m2, a) \
	amountdef = get_nc(m1, f, art24) * ass.DR; \
	amountimm = amountdef; \
	ic_nc->m2[a] = amountdef * probfactdef + amountimm * probfactimm;
#define ASSETS(m, a) \
	amountdef = get_assets(a, DEF, res, red_cap, f); \
	amountimm = get_assets(a, IMM, res, red_cap, f); \
	assets->m = amountdef * probfactdef + amountimm * probfactimm;

void set_dbo_ret(struct retirement *dbo,
		const double art24[const static METHOD_AMOUNT],
		double res, double red_cap, struct factor f)
{
	double probfactdef = 0.0; // probability factors for deferred payment
	double probfactimm = 0.0; // probability factors for immediate payment
	double amountdef = 0.0;
	double amountimm = 0.0;

	probfactdef = f.wxdef * f.kPx * f.nPk * f.vn;
	probfactimm = (f.wximm + f.retx) * f.kPx * f.vk;

	DBO(PUC, puc, PAR115);
	DBO(PUC, puc, PAR113);
	DBO(PUC, puc, MATHRES);
	DBO(TUC, tuc, PAR115);
	DBO(TUC, tuc, PAR113);
	DBO(TUC, tuc, MATHRES);
}

void set_nc_ret(struct retirement *nc, const double art24[const static
		METHOD_AMOUNT], struct factor f)
{
	double probfactdef = 0.0; // probability factors for deferred payment
	double probfactimm = 0.0; // probability factors for immediate payment
	double amountdef = 0.0;
	double amountimm = 0.0;

	probfactdef = f.wxdef * f.kPx * f.nPk * f.vn;
	probfactimm = (f.wximm + f.retx) * f.kPx * f.vk;

	NC(PUC, puc, PAR115);
	NC(PUC, puc, PAR113);
	NC(PUC, puc, MATHRES);
	NC(TUC, tuc, PAR115);
	NC(TUC, tuc, PAR113);
	NC(TUC, tuc, MATHRES);
}

void set_ic_nc_ret(struct retirement *ic_nc, const double art24[const static
		METHOD_AMOUNT], struct factor f)
{
	double probfactdef = 0.0; // probability factors for deferred payment
	double probfactimm = 0.0; // probability factors for immediate payment
	double amountdef = 0.0;
	double amountimm = 0.0;

	probfactdef = f.wxdef * f.kPx * f.nPk * f.vn;
	probfactimm = (f.wximm + f.retx) * f.kPx * f.vk;

	IC_NC(PUC, puc, PAR115);
	IC_NC(PUC, puc, PAR113);
	IC_NC(PUC, puc, MATHRES);
	IC_NC(TUC, tuc, PAR115);
	IC_NC(TUC, tuc, PAR113);
	IC_NC(TUC, tuc, MATHRES);
}

void set_assets(struct assets *assets, double res, double red_cap,
		struct factor f)
{
	double probfactdef = 0.0; // probability factors for deferred payment
	double probfactimm = 0.0; // probability factors for immediate payment
	double amountdef = 0.0;
	double amountimm = 0.0;

	probfactdef = f.wxdef * f.kPx * f.nPk * f.vn;
	probfactimm = (f.wximm + f.retx) * f.kPx * f.vk;

	ASSETS(par115, PAR115);
	ASSETS(par113, PAR113);
	ASSETS(math_res, MATHRES);
}

#undef DBO
#undef NC
#undef IC_NC
#undef ASSETS

static void update_EBP_death(CurrentMember *cm, int k)
{
	const struct date *DOC_1 = cm->proj[1].DOC;
	const struct date *DOC_k = cm->proj[k].DOC;
	int year = calcyears(DOC_1, DOC_k, 0);
	struct factor f = cm->proj[k].factor;
	struct date *IMMdate = newDate(0, DOC_1->year + year, DOC_1->month, 1);
	double vk = 0.0;
	double capdth = 0.0;
	double probs = 0.0;
	if (year >= 0) {
		vk = pow(1 + ass.DR, -calcyears(IMMdate, DOC_k, 0));
		capdth = cm->proj[k].death_risk + cm->proj[k].death_res;
		probs = f.qx * f.kPx * vk;
		cm->proj[year+1].ebp_death[TBO] += capdth * probs;
		cm->proj[year+1].ebp_death[PBO] += capdth * f.ff * probs;
		cm->proj[year+1].pbo_nc_death += capdth * f.ff_sc * probs;
	}
	free(IMMdate);
}

#define EBP_RET(m1, m2, a, cf, p1, p2) \
	amount##p2 = get_dbo(m1, a, p1, cf, art24, res, red_cap, f); \
	cm->proj[year##p1+1].ebp_ret[cf].m2[a] += amount##p2 * probfact##p2;
#define PBO_NC(m1, m2, a, p1, p2) \
	amount##p2 = get_nc(m1, f, art24); \
	cm->proj[year##p1+1].pbo_nc_ret.m2[a] += amount##p2 * probfact##p2;

void update_EBP_ret(CurrentMember *cm, int k,
		const double art24[const static METHOD_AMOUNT],
		double res, double red_cap)
{
	double probfactdef = 0.0;
	double probfactimm = 0.0;
	double amountdef = 0.0;
	double amountimm = 0.0;
	double vIMM = 0.0;
	double vDEF = 0.0;
	int yearIMM = 0;
	int yearDEF = 0;
	struct date *Ndate = 0;
	struct date *IMMdate = 0;
	struct date *DEFdate = 0;
	struct factor f = cm->proj[k].factor;

	Ndate = newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1);
	yearIMM = calcyears(cm->proj[1].DOC, cm->proj[k].DOC, 0);
	IMMdate = newDate(0, cm->proj[1].DOC->year + yearIMM, cm->proj[1].DOC->month, 1);
	yearDEF = MAX(0.0, calcyears(cm->proj[1].DOC, Ndate, 0));
	DEFdate = newDate(0, cm->proj[1].DOC->year + yearDEF, cm->proj[1].DOC->month, 1);

	if (yearIMM >= 0) {
		vIMM = pow(1 + ass.DR, -calcyears(IMMdate, cm->proj[k].DOC, 0));
		probfactimm = (f.wximm + f.retx) * f.kPx * vIMM;

		EBP_RET(PUC, puc, PAR115, PBO, IMM, imm);
		EBP_RET(PUC, puc, PAR115, TBO, IMM, imm);
		PBO_NC(PUC, puc, PAR115, IMM, imm);
		EBP_RET(PUC, puc, PAR113, PBO, IMM, imm);
		EBP_RET(PUC, puc, PAR113, TBO, IMM, imm);
		PBO_NC(PUC, puc, PAR113, IMM, imm);
		EBP_RET(PUC, puc, MATHRES, PBO, IMM, imm);
		EBP_RET(PUC, puc, MATHRES, TBO, IMM, imm);
		PBO_NC(PUC, puc, MATHRES, IMM, imm);
		EBP_RET(TUC, tuc, PAR115, PBO, IMM, imm);
		EBP_RET(TUC, tuc, PAR115, TBO, IMM, imm);
		PBO_NC(TUC, tuc, PAR115, IMM, imm);
		EBP_RET(TUC, tuc, PAR113, PBO, IMM, imm);
		EBP_RET(TUC, tuc, PAR113, TBO, IMM, imm);
		PBO_NC(TUC, tuc, PAR113, IMM, imm);
		EBP_RET(TUC, tuc, MATHRES, PBO, IMM, imm);
		EBP_RET(TUC, tuc, MATHRES, TBO, IMM, imm);
		PBO_NC(TUC, tuc, MATHRES, IMM, imm);
	}

	vDEF = pow(1 + ass.DR, -calcyears(DEFdate, Ndate, 0));
	probfactdef = f.wxdef * f.nPk * f.kPx * vDEF;

	EBP_RET(PUC, puc, PAR115, PBO, DEF, def);
	EBP_RET(PUC, puc, PAR115, TBO, DEF, def);
	PBO_NC(PUC, puc, PAR115, DEF, def);
	EBP_RET(PUC, puc, PAR113, PBO, DEF, def);
	EBP_RET(PUC, puc, PAR113, TBO, DEF, def);
	PBO_NC(PUC, puc, PAR113, DEF, def);
	EBP_RET(PUC, puc, MATHRES, PBO, DEF, def);
	EBP_RET(PUC, puc, MATHRES, TBO, DEF, def);
	PBO_NC(PUC, puc, MATHRES, DEF, def);
	EBP_RET(TUC, tuc, PAR115, PBO, DEF, def);
	EBP_RET(TUC, tuc, PAR115, TBO, DEF, def);
	PBO_NC(TUC, tuc, PAR115, DEF, def);
	EBP_RET(TUC, tuc, PAR113, PBO, DEF, def);
	EBP_RET(TUC, tuc, PAR113, TBO, DEF, def);
	PBO_NC(TUC, tuc, PAR113, DEF, def);
	EBP_RET(TUC, tuc, MATHRES, PBO, DEF, def);
	EBP_RET(TUC, tuc, MATHRES, TBO, DEF, def);
	PBO_NC(TUC, tuc, MATHRES, DEF, def);

	free(Ndate);
	free(IMMdate);
	free(DEFdate);
}

#undef EBP_RET
#undef PBO_NC

void update_EBP(CurrentMember *cm, int k,
		const double art24[const static METHOD_AMOUNT],
		double res, double red_cap)
{
	update_EBP_death(cm, k);
	update_EBP_ret(cm, k, art24, res, red_cap);
}
