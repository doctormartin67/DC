#include <math.h>
#include "actfuncs.h"
#include "lifetables.h"
#include "helperfunctions.h"
#include "calculate.h"
#include "errorexit.h"
#include "assumptions.h"

/*
 * these tables need updating to be more general !!! TODO
 */
const char *lxmr = "/home/doctormartin67/Coding/tables/tables/LXMR";
const char *lxfr = "/home/doctormartin67/Coding/tables/tables/LXFR";

static struct date *get_DOC(const CurrentMember *cm, int k)
{
	struct date *d = 0, *Ndate = 0, *docdate = 0;
	unsigned month_k = cm->proj[k].DOC->month;
	unsigned year_k = cm->proj[k].DOC->year;

	Ndate = newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1);
	if (in_prolongation(cm, k)) {
		unsigned addyear = 0;
		unsigned month_1 = cm->proj[1].DOC->month;
		addyear = (month_k >= month_1) ? 1 : 0;
		docdate = newDate(0, year_k + addyear, month_1, 1);
		d = MINDATE2(Ndate, docdate);
	} else {
		docdate = newDate(0, year_k + 1, month_k, 1);
		d = MINDATE3(Ndate, docdate, cm->DOR);
		if (d == cm->DOR) {
			d = Datedup(cm->DOR);
		}
	}
	if (0 == Ndate || 0 == docdate) die("invalid date");

	return d;
}

static void init_age(CurrentMember *cm)
{
	for (size_t k = 0; k < MAXPROJ + 1; k++) {
		cm->proj[k].age = calcyears(cm->DOB, cm->proj[k].DOC, 1);
	}
}

static void init_DOC(CurrentMember *cm)
{
	for (size_t k = 0; k < MAXPROJ + 1; k++) {
		switch (k) {
			case 0:
				cm->proj[k].DOC = Datedup(cm->DOS);
				break;
			case 1:
				cm->proj[k].DOC = Datedup(ass.DOC);
				break;
			default:
				cm->proj[k].DOC = get_DOC(cm, k - 1);
				break;
		}
	}
}

static void init_service(CurrentMember *cm)
{
	assert(cm);
	struct date *doc = 0;
	const struct date *doe = cm->DOE;
	const struct date *doa = cm->DOA;

	for (size_t k = 0; k < MAXPROJ; k++) {
		doc = cm->proj[k].DOC;
		cm->proj[k].nDOE = calcyears(doe, doc, (1 == doe->day ? 0 : 1));
		cm->proj[k].nDOA = calcyears(doa, doc, (1 == doa->day ? 0 : 1));
	}
}

static void init_premiums(CurrentMember *cm)
{
	double *prem = 0;
	for (size_t l = 0; l < EREE_AMOUNT; l++) {
		prem = &cm->proj[0].gens[MAXGEN - 1].premium[l];
		*prem = (l == ER ? calcA(cm, 0) : calcC(cm, 0));
		for (size_t j = 0; j < MAXGEN - 1; j++) {
			*prem = MAX(0.0, *prem
					- cm->proj[0].gens[j].premium[l]);
		}
	}
}

static void init_death(CurrentMember *cm)
{
	if (UKMS == cm->tariff) {
		cm->proj[0].death_res
			= gen_sum(cm->proj[0].gens, ER, RES, PUC) 
			+ gen_sum(cm->proj[0].gens, EE, RES, PUC) 
			+ gen_sum(cm->proj[0].gens, ER, RESPS, PUC) 
			+ gen_sum(cm->proj[0].gens, EE, RESPS, PUC);
	} else {
		cm->proj[0].death_res = 0.0;
	}

	cm->proj[0].death_risk = calcDTH(cm, 0); 
}

static void init_cm(CurrentMember *cm)
{
	assert(cm);

	init_DOC(cm);
	init_age(cm);
	init_service(cm);

	cm->proj[1].factor.kPx = 1;

	init_death(cm);
	init_premiums(cm);

	set_tariffs(cm);
}

static void prolongate(struct projection *p)
{
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		p->gens[MAXGEN-1].premium[i]
			= gen_sum(p->gens, i, PREMIUM, PUC);
		p->gens[MAXGEN-1].lump_sums.death_lump_sum[i]
			= gen_sum(p->gens, i, CAPDTH, PUC);
		p->gens[MAXGEN-1].reserves.res[i].puc
			= gen_sum(p->gens, i, RES, PUC);
		p->gens[MAXGEN-1].reserves.ps[i]
			= gen_sum(p->gens, i, RESPS, 0);
		p->delta_cap[i] = 0;
		for (int j = 0; j < MAXGEN-1; j++) {
			p->gens[j].premium[i] = 0;
			p->gens[j].lump_sums.death_lump_sum[i] = 0;
			p->gens[j].reserves.res[i].puc = 0;
			p->gens[j].reserves.ps[i] = 0;
		}
	}
}

static double next_sal(CurrentMember *cm, int k)
{
	double ss = salaryscale(cm, k);
	double sal = cm->proj[k-1].sal;
	unsigned year1 = cm->proj[k].DOC->year;
	unsigned year0 = cm->proj[k-1].DOC->year;
	if (1 == k) {
		return sal * pow(1 + ss, year1 - year0 + ass.incrSalk1);
	} else {
		return sal * pow(1 + ss, year1 - year0);
	}
}

static double next_ff(CurrentMember *cm, int k)
{
	double ERprem = 0.0;
	double EEprem = 0.0;
	double nDOA = 0.0;

	ERprem = gen_sum(cm->proj[1].gens, ER, PREMIUM, PUC);
	EEprem = gen_sum(cm->proj[1].gens, EE, PREMIUM, PUC);

	if (!(cm->status & ACT) || 0 == ERprem + EEprem) {
		return 1;
	} else {
		nDOA = (0 == cm->proj[k].nDOA ? 1 : cm->proj[k].nDOA);
		return cm->proj[1].nDOA / nDOA;
	}
}

static double next_ff_sc(CurrentMember *cm, int k)
{
	double ERprem = 0.0;
	double EEprem = 0.0;
	double nDOA = 0.0;

	ERprem = gen_sum(cm->proj[1].gens, ER, PREMIUM, PUC);
	EEprem = gen_sum(cm->proj[1].gens, EE, PREMIUM, PUC);

	if (!(cm->status & ACT) || 0 == ERprem + EEprem || 1 == k) {
		return 0.0;
	} else {
		nDOA = (0 == cm->proj[k].nDOA ? 1 : cm->proj[k].nDOA);
		return (cm->proj[2].age - cm->proj[1].age) / nDOA;

	}
}

static void update_factors(CurrentMember *cm, int k)
{
	double wx = 0.0;
	double age0 = cm->proj[k-1].age;
	double age1 = cm->proj[k].age;
	double age2 = cm->proj[k+1].age;
	double periodk = cm->proj[k].age - cm->proj[1].age;
	double periodNRA = NRA(cm, k) - cm->proj[1].age;
	const char *table = 0;

	cm->proj[k].factor.ff = next_ff(cm, k);
	cm->proj[k].factor.ff_sc = next_ff_sc(cm, k);

	wx = wxdef(cm, k) * (age2 - age1);
	cm->proj[k].factor.wxdef = wx * ass.TRM_PercDef;
	cm->proj[k].factor.wximm = wx * (1 - ass.TRM_PercDef);

	if (cm->status & MALE) {
		table = lxmr;
	} else {
		table = lxfr;
	}

	cm->proj[k].factor.qx = 1-npx(table, age1, age2, ass.agecorr);
	cm->proj[k].factor.retx = retx(cm, k) * (k > 1 && age1 == age0 ? 0 : 1);
	cm->proj[k].factor.nPk = npx(table, cm->proj[k].age, NRA(cm, k),
			ass.agecorr);

	periodk = cm->proj[k].age - cm->proj[1].age;
	periodNRA = NRA(cm, k) - cm->proj[1].age;
	cm->proj[k].factor.vk = pow(1 + ass.DR, -periodk);
	cm->proj[k].factor.vn = pow(1 + ass.DR, -periodNRA);    
	cm->proj[k].factor.vk113 = pow(1 + ass.DR113, -periodk);
	cm->proj[k].factor.vn113 = pow(1 + ass.DR113, -periodNRA);    
}

static void update_death(CurrentMember *cm, int k, double res)
{

	struct factor f = cm->proj[k].factor;
	double probs = f.ff * f.qx * f.kPx * f.vk;
	double probsSC = f.ff_sc * f.qx * f.kPx * f.vk;
	cm->proj[k].death_res = (UKMS == cm->tariff ? res : 0);
	cm->proj[k].death_risk = calcDTH(cm, k); 
	cm->proj[k].dbo_death.death_res = cm->proj[k].death_res * probs;
	cm->proj[k].dbo_death.death_risk = cm->proj[k].death_risk * probs;
	cm->proj[k].nc_death.death_res = cm->proj[k].death_res * probsSC;
	cm->proj[k].nc_death.death_risk = cm->proj[k].death_risk * probsSC;
	cm->proj[k].nc_death.ic_death_res = 
		cm->proj[k].death_res * probsSC * ass.DR; 
	cm->proj[k].nc_death.ic_death_risk = 
		cm->proj[k].death_risk * probsSC * ass.DR; 
}

void run_member(CurrentMember *cm)
{
	assert(cm);
	double ART24TOT[METHOD_AMOUNT] = {0};
	double RESTOT[METHOD_AMOUNT] = {0};
	double red_cap = 0.0;
	struct factor f = (struct factor){0};

	init_cm(cm);

	for (size_t k = 1; k < MAXPROJ; k++) {

		if (NRA(cm, k-1) == cm->proj[k-1].age) {
			break;
		}

		if (!datecmp(cm->proj[k-1].DOC, cm->DOR)) {
			prolongate(&cm->proj[k-1]);
		}

		cm->proj[k].sal = next_sal(cm, k);

		update_death_lump_sums(cm, k - 1);
		update_res_lump_sums(cm, k - 1); 
		update_premiums(cm, k - 1);
		update_art24(cm, k - 1);

		for (size_t j = 0; j < METHOD_AMOUNT; j++) {
			ART24TOT[j] = gen_sum_art24(cm->proj[k].art24, j);
			RESTOT[j] = gen_sum(cm->proj[k].gens, ER, RES, j)
				+ gen_sum(cm->proj[k].gens, EE, RES, j)
				+ gen_sum(cm->proj[k].gens, ER, RESPS, j)
				+ gen_sum(cm->proj[k].gens, EE, RESPS, j);
		}

		red_cap = gen_sum(cm->proj[k].gens, ER, CAPRED, TUC)
			+ gen_sum(cm->proj[k].gens, EE, CAPRED, TUC);

		update_factors(cm, k);

		f = cm->proj[k].factor;
		set_dbo_ret(&cm->proj[k].dbo_ret, ART24TOT, RESTOT[TUC],
				red_cap, f);
		set_nc_ret(&cm->proj[k].nc_ret, ART24TOT, f);
		set_ic_nc_ret(&cm->proj[k].ic_nc_ret, ART24TOT, f);
		set_assets(&cm->proj[k].assets, RESTOT[TUC], red_cap, f);

		if (1 == k) {
			cm->proj[k].assets.math_res = RESTOT[PUC];
		} else {
			cm->proj[k].assets.math_res = 0.0;
		}

		cm->proj[k].afsl = (f.qx + f.wxdef + f.wximm + f.retx) * f.kPx
			* (cm->proj[k].age - cm->proj[1].age);
		
		update_death(cm, k, RESTOT[PUC]);
		update_EBP(cm, k, ART24TOT, RESTOT[TUC], red_cap);

		if (k + 1 < MAXPROJ) {
			cm->proj[k+1].factor.kPx = f.kPx * (1 - f.qx)
				* (1 - f.wxdef - f.wximm) * (1 - f.retx);
		}
	}     
}
