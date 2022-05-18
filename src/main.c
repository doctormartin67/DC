#include <math.h>
#include "actuarialfunctions.h"
#include "lifetables.h"
#include "helperfunctions.h"
#include "calculate.h"
#include "errorexit.h"
#include "assumptions.h"

extern void userinterface();
extern void runmember(CurrentMember cm[static 1]);

static void init_cm(CurrentMember cm[static 1])
{
	struct date *doc = 0;
	const struct date *dob = cm->DOB;
	const struct date *doe = cm->DOE;
	const struct date *doa = cm->DOA;
	double *prem = 0;

	assert(cm);

	//-  Dates and age  -
	cm->proj[0].DOC = Datedup(cm->DOS);
	cm->proj[1].DOC = Datedup(ass.DOC);
	doc = cm->proj[0].DOC;
	cm->proj[0].age = calcyears(dob, doc, 1);
	cm->proj[0].nDOE = calcyears(doe, doc, (1 == doe->day ? 0 : 1));
	cm->proj[0].nDOA = calcyears(doa, doc, (1 == doa->day ? 0 : 1));

	cm->proj[1].factor.kPx = 1;

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

	//-  Premium  -
	for (size_t l = 0; l < EREE_AMOUNT; l++) {
		prem = &cm->proj[0].gens[MAXGEN - 1].premium[l];
		*prem = (l == ER ? calcA(cm, 0) : calcC(cm, 0));
		for (size_t j = 0; j < MAXGEN-1; j++) {
			*prem = MAX(0.0, *prem
					- cm->proj[0].gens[j].premium[l]);
		}
	}

	set_tariffs(cm);
}

static struct date *getDOC(const CurrentMember cm[static 1], int k)
{
	unsigned month_k = cm->proj[k].DOC->month;
	unsigned year_k = cm->proj[k].DOC->year;
	struct date *d = 0, *Ndate = 0, *docdate = 0;

	Ndate = newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1);
	docdate = newDate(0, year_k + 1, month_k, 1);

	if (0 == Ndate || 0 == docdate) die("invalid date");

	d = MINDATE3(Ndate, docdate, cm->DOR);

	if (d == cm->DOR)
		d = Datedup(cm->DOR);

	return d;
}

static struct date *getDOC_prolongation(const CurrentMember cm[static 1],
		int k)
{
	unsigned addyear = 0;
	struct date *d = 0, *Ndate = 0, *docdate = 0;

	unsigned month_1 = cm->proj[1].DOC->month;
	unsigned month_k = cm->proj[k].DOC->month;
	unsigned year_k = cm->proj[k].DOC->year;
	addyear = (month_k >= month_1) ? 1 : 0;
	Ndate = newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1);
	docdate = newDate(0, year_k + addyear, month_1, 1);

	if (0 == Ndate || 0 == docdate) die("invalid date");

	d = MINDATE2(Ndate, docdate);

	return d;
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

void runmember(CurrentMember cm[static 1])
{
	double ERprem = 0.0;
	double EEprem = 0.0;
	double nDOA = 0.0;
	double wx = 0.0;
	double TRMDef = ass.TRM_PercDef;
	double TRMImm = 1 - ass.TRM_PercDef;
	double periodk = 0.0;
	double periodNRA = 0.0;
	double probs = 0.0;
	double probsSC = 0.0;
	double ART24TOT[METHOD_AMOUNT] = {0};
	double RESTOT[METHOD_AMOUNT] = {0};
	double REDCAPTOT[METHOD_AMOUNT] = {0};

	init_cm(cm);

	for (size_t k = 1; k < MAXPROJ; k++) {
		if (1 == k) {
			cm->proj[k+1].DOC = getDOC(cm, k);
		} else if (1 < k && MAXPROJBEFOREPROL > k) {
			cm->proj[k].DOC = getDOC(cm, k - 1);
			cm->proj[k+1].DOC = getDOC(cm, k);
		} else if (MAXPROJBEFOREPROL == k) {
			cm->proj[k].DOC = getDOC(cm, k - 1);
			cm->proj[k+1].DOC = getDOC_prolongation(cm, k);		
		} else if (MAXPROJBEFOREPROL < k) {
			cm->proj[k].DOC = getDOC_prolongation(cm, k - 1);
			cm->proj[k+1].DOC = getDOC_prolongation(cm, k);		
			if (MAXPROJBEFOREPROL + 1 == k)
				prolongate(&cm->proj[k-1]);
		}

		cm->proj[k].age = calcyears(cm->DOB, cm->proj[k].DOC, 1);
		cm->proj[k+1].age = calcyears(cm->DOB, cm->proj[k+1].DOC, 1);
		cm->proj[k].nDOA = calcyears(cm->DOA, cm->proj[k].DOC, 
				(cm->DOA->day == 1 ? 0 : 1));
		cm->proj[k].nDOE = calcyears(cm->DOE, cm->proj[k].DOC, 
				(cm->DOE->day == 1 ? 0 : 1));

		cm->proj[k].sal = cm->proj[k-1].sal
			* pow((1 + salaryscale(cm, k)),
					cm->proj[k].DOC->year 
					- cm->proj[k-1].DOC->year 
					+ (1 == k ? ass.incrSalk1 : 0));


		update_death_lump_sums(cm, k - 1);
		update_res_lump_sums(cm, k - 1); 
		update_premiums(cm, k - 1);
		update_art24(cm, k - 1);

		for (int j = 0; j < METHOD_AMOUNT; j++) {
			ART24TOT[j] = gen_sum_art24(cm->proj[k].art24, j);
			RESTOT[j] = gen_sum(cm->proj[k].gens, ER, RES, j)
				+ gen_sum(cm->proj[k].gens, EE, RES, j)
				+ gen_sum(cm->proj[k].gens, ER, RESPS, j)
				+ gen_sum(cm->proj[k].gens, EE, RESPS, j);
			REDCAPTOT[j] = gen_sum(cm->proj[k].gens, ER, CAPRED, j)
				+ gen_sum(cm->proj[k].gens, EE, CAPRED, j);
		}

		ERprem = gen_sum(cm->proj[1].gens, ER, PREMIUM, PUC);
		EEprem = gen_sum(cm->proj[1].gens, EE, PREMIUM, PUC);
		if (!(cm->status & ACT) || 0 == ERprem + EEprem) {
			cm->proj[k].factor.ff = 1;
			cm->proj[k].factor.ff_sc = 0;
		} else {
			nDOA = (0 == cm->proj[k].nDOA ? 1 : cm->proj[k].nDOA);
			cm->proj[k].factor.ff = cm->proj[1].nDOA / nDOA;
			if (1 == k) {
				cm->proj[k].factor.ff_sc = 0.0; 
			} else {
				cm->proj[k].factor.ff_sc
					= (cm->proj[2].age - cm->proj[1].age) / nDOA;
			}

		}

		wx = wxdef(cm, k) * (cm->proj[k+1].age - cm->proj[k].age);
		cm->proj[k].factor.wxdef = wx * TRMDef;
		cm->proj[k].factor.wximm = wx * TRMImm;

		cm->proj[k].factor.qx
			= 1 - npx((cm->status & MALE ? LXMR : LXFR),
					cm->proj[k].age, cm->proj[k+1].age, ass.agecorr);
		cm->proj[k].factor.retx = retx(cm, k) 
			* (k > 1 && cm->proj[k].age == cm->proj[k-1].age ? 0 : 1);
		cm->proj[k].factor.nPk = npx((cm->status & MALE ? LXMR : LXFR),
				cm->proj[k].age, NRA(cm, k), ass.agecorr);

		periodk = cm->proj[k].age - cm->proj[1].age;
		periodNRA = NRA(cm, k) - cm->proj[1].age;
		cm->proj[k].factor.vk = pow(1 + ass.DR, -periodk);
		cm->proj[k].factor.vn = pow(1 + ass.DR, -periodNRA);    
		cm->proj[k].factor.vk113 = pow(1 + ass.DR113, -periodk);
		cm->proj[k].factor.vn113 = pow(1 + ass.DR113, -periodNRA);    

		set_dbo_ret(&cm->proj[k].dbo_ret, ART24TOT, RESTOT[TUC],
				REDCAPTOT[TUC], cm->proj[k].factor);
		set_nc_ret(&cm->proj[k].nc_ret, ART24TOT, cm->proj[k].factor);
		set_ic_nc_ret(&cm->proj[k].ic_nc_ret, ART24TOT, cm->proj[k].factor);
		set_assets(&cm->proj[k].assets, RESTOT[TUC], REDCAPTOT[TUC],
				cm->proj[k].factor);

		cm->proj[k].afsl = cm->proj[k-1].afsl
			* (cm->proj[k].factor.wxdef + cm->proj[k].factor.wximm
			* + cm->proj[k].factor.retx)
			* cm->proj[k].factor.kPx * periodk; 

		cm->proj[k].death_res = (UKMS == cm->tariff ? RESTOT[PUC] : 0);
		cm->proj[k].death_risk = calcDTH(cm, k); 
		probs = cm->proj[k].factor.ff
			* cm->proj[k].factor.qx
			* cm->proj[k].factor.kPx * cm->proj[k].factor.vk;
		probsSC = cm->proj[k].factor.ff_sc * cm->proj[k].factor.qx
			* cm->proj[k].factor.kPx * cm->proj[k].factor.vk;
		cm->proj[k].dbo_death.death_res = cm->proj[k].death_res * probs; 
		cm->proj[k].dbo_death.death_risk = cm->proj[k].death_risk * probs; 
		cm->proj[k].nc_death.death_res = cm->proj[k].death_res * probsSC; 
		cm->proj[k].nc_death.death_risk = cm->proj[k].death_risk * probsSC; 
		cm->proj[k].nc_death.ic_death_res = 
			cm->proj[k].death_res * probsSC * ass.DR; 
		cm->proj[k].nc_death.ic_death_risk = 
			cm->proj[k].death_risk * probsSC * ass.DR; 

		update_EBP(cm, k, ART24TOT, RESTOT[TUC], REDCAPTOT[TUC]);

		if (k + 1 < MAXPROJ) {
			cm->proj[k+1].factor.kPx
				= cm->proj[k].factor.kPx
				* (1 - cm->proj[k].factor.qx)
				* (1 - cm->proj[k].factor.wxdef
						- cm->proj[k].factor.wximm)
				* (1 - cm->proj[k].factor.retx);
		}
	}     
}

int main(void)
{
	makeLifeTables();
	init_builtin_vars();
	userinterface();
	return 0;
}
