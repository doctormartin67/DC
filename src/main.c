#include "helperfunctions.h"
#include "actuarialfunctions.h"
#include "errorexit.h"
#include "assumptions.h"

void userinterface();
void runmember(CurrentMember cm[static 1]);
static void init_cm(CurrentMember *);
static struct date *getDOC(const CurrentMember cm[static 1], int k);
static struct date *getDOC_prolongation(const CurrentMember cm[static 1],
		int k);
static void prolongate(CurrentMember cm[static 1], int k);

int main(void)
{
	makeLifeTables();

	userinterface();
	return 0;
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

	setparameters(cm, 0);
	init_cm(cm);

	for (int k = 1; k < MAXPROJ; k++) {
		if (1 == k) {
			cm->DOC[k+1] = getDOC(cm, k);
		} else if (1 < k && MAXPROJBEFOREPROL > k) {
			free(cm->DOC[k]);
			cm->DOC[k] = getDOC(cm, k - 1);
			cm->DOC[k+1] = getDOC(cm, k);
		} else if (MAXPROJBEFOREPROL == k) {
			free(cm->DOC[k]);
			cm->DOC[k] = getDOC(cm, k - 1);
			cm->DOC[k+1] = getDOC_prolongation(cm, k);		
		} else if (MAXPROJBEFOREPROL < k) {
			free(cm->DOC[k]);
			cm->DOC[k] = getDOC_prolongation(cm, k - 1);
			cm->DOC[k+1] = getDOC_prolongation(cm, k);		
			if (MAXPROJBEFOREPROL + 1 == k)
				prolongate(cm, k - 1);
		}

		cm->age[k] = calcyears(cm->DOB, cm->DOC[k], 1);
		cm->age[k+1] = calcyears(cm->DOB, cm->DOC[k+1], 1);
		cm->nDOA[k] = calcyears(cm->DOA, cm->DOC[k], 
				(cm->DOA->day == 1 ? 0 : 1));
		cm->nDOE[k] = calcyears(cm->DOE, cm->DOC[k], 
				(cm->DOE->day == 1 ? 0 : 1));

		cm->sal[k] = cm->sal[k-1] * pow((1 + salaryscale(cm, k)),
				cm->DOC[k]->year 
				- cm->DOC[k-1]->year 
				+ (1 == k ? ass.incrSalk1 : 0));

		setparameters(cm, k);

		evolCAPDTH(cm, k - 1);
		evolRES(cm, k - 1); 
		evolPremiums(cm, k - 1);
		evolART24(cm, k - 1);

		for (int j = 0; j < METHOD_AMOUNT; j++) {
			ART24TOT[j] = cm->ART24[j][ER][ART24GEN1][k]
				+ cm->ART24[j][ER][ART24GEN2][k]
				+ cm->ART24[j][EE][ART24GEN1][k]
				+ cm->ART24[j][EE][ART24GEN2][k];
			RESTOT[j] = gensum(cm->RES[j], ER, k)
				+ gensum(cm->RES[j], EE, k)
				+ gensum(cm->RESPS[j], ER, k)
				+ gensum(cm->RESPS[j], EE, k);
			REDCAPTOT[j] = gensum(cm->REDCAP[j], ER, k)
				+ gensum(cm->REDCAP[j], EE, k);
		}

		ERprem = gensum(cm->PREMIUM, ER, 1);
		EEprem = gensum(cm->PREMIUM, EE, 1);
		if (!(cm->status & ACT) || 0 == ERprem + EEprem) {
			cm->FF[k] = 1;
			cm->FFSC[k] = 0;
		} else {
			nDOA = (0 == cm->nDOA[k] ? 1 : cm->nDOA[k]);
			cm->FF[k] = cm->nDOA[1] / nDOA;
			if (1 == k)
				cm->FFSC[k] = 0.0; 
			else
				cm->FFSC[k] = (cm->age[2] - cm->age[1]) / nDOA;

		}

		wx = wxdef(cm, k) * (cm->age[k+1] - cm->age[k]);
		cm->wxdef[k] = wx * TRMDef;
		cm->wximm[k] = wx * TRMImm;

		cm->qx[k] = 1 - npx((cm->status & MALE ? LXMR : LXFR),
				cm->age[k], cm->age[k+1], ass.agecorr);
		cm->retx[k] = retx(cm, k) 
			* (k > 1 && cm->age[k] == cm->age[k-1] ? 0 : 1);
		cm->nPk[k] = npx((cm->status & MALE ? LXMR : LXFR),
				cm->age[k], NRA(cm, k), ass.agecorr);

		periodk = cm->age[k] - cm->age[1];
		periodNRA = NRA(cm, k) - cm->age[1];
		cm->vk[k] = pow(1 + ass.DR, -periodk);
		cm->vn[k] = pow(1 + ass.DR, -periodNRA);    
		cm->vk113[k] = pow(1 + ass.DR113, -periodk);
		cm->vn113[k] = pow(1 + ass.DR113, -periodNRA);    

		evolDBONCIC(cm, k, ART24TOT, RESTOT, REDCAPTOT);

		cm->AFSL[k] = cm->AFSL[k-1]
			* (cm->wxdef[k] + cm->wximm[k] + cm->retx[k])
			* cm->kPx[k] * periodk; 

		cm->CAPDTHRESPart[k] = (UKMS == cm->tariff ? RESTOT[PUC] : 0);
		cm->CAPDTHRiskPart[k] = calcDTH(cm, k); 
		probs = cm->FF[k] * cm->qx[k] * cm->kPx[k] * cm->vk[k];
		probsSC = cm->FFSC[k] * cm->qx[k] * cm->kPx[k] * cm->vk[k];
		cm->DBODTHRESPart[k] = cm->CAPDTHRESPart[k] * probs; 
		cm->DBODTHRiskPart[k] = cm->CAPDTHRiskPart[k] * probs; 
		cm->NCDTHRESPart[k] = cm->CAPDTHRESPart[k] * probsSC; 
		cm->NCDTHRiskPart[k] = cm->CAPDTHRiskPart[k] * probsSC; 
		cm->ICNCDTHRESPart[k] = 
			cm->CAPDTHRESPart[k] * probsSC * ass.DR; 
		cm->ICNCDTHRiskPart[k] = 
			cm->CAPDTHRiskPart[k] * probsSC * ass.DR; 

		evolEBP(cm, k, ART24TOT, RESTOT, REDCAPTOT);

		if (k + 1 < MAXPROJ) {
			cm->kPx[k+1] = cm->kPx[k] * (1 - cm->qx[k])
			* (1 - cm->wxdef[k] - cm->wximm[k])
			* (1 - cm->retx[k]);
		}
	}     
}

static void init_cm(CurrentMember cm[static 1])
{
	struct date **doc = cm->DOC;
	const struct date *dob = cm->DOB;
	const struct date *doe = cm->DOE;
	const struct date *doa = cm->DOA;
	double *prem = 0;

	//-  Dates and age  -
	doc[0] = Datedup(cm->DOS);
	doc[1] = Datedup(ass.DOC);
	cm->age[0] = calcyears(dob, *doc, 1);
	cm->nDOE[0] = calcyears(doe, *doc, (1 == doe->day ? 0 : 1));
	cm->nDOA[0] = calcyears(doa, *doc, (1 == doa->day ? 0 : 1));

	cm->kPx[1] = 1;

	cm->CAPDTHRESPart[0] = 
		(cm->tariff == UKMS ? 
		 gensum(cm->RES[PUC], ER, 0) 
		 + gensum(cm->RES[PUC], EE, 0)
		 + gensum(cm->RESPS[PUC], ER, 0) 
		 + gensum(cm->RESPS[PUC], EE, 0) : 0);
	cm->CAPDTHRiskPart[0] = calcDTH(cm, 0); 

	//-  Premium  -
	for (int l = 0; l < EREE_AMOUNT; l++) {
		prem = cm->PREMIUM[l][MAXGEN-1];
		*prem = (l == ER ? calcA(cm, 0) : calcC(cm, 0));
		for (int j = 0; j < MAXGEN-1; j++) {
			prem = cm->PREMIUM[l][MAXGEN-1];
			*prem = MAX2(0.0, *prem - *cm->PREMIUM[l][j]);
		}
	}
	
	set_tariffs(cm);
}

static struct date *getDOC(const CurrentMember cm[static 1], int k)
{
	struct date *d = 0, *Ndate = 0, *docdate = 0;

	Ndate = newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1);
	docdate = newDate(0, cm->DOC[k]->year + 1, cm->DOC[k]->month, 1);

	if (0 == Ndate || 0 == docdate) die("invalid date");

	d = MINDATE3(Ndate, docdate, cm->DOR);

	if (d != Ndate) free(Ndate);
	if (d != docdate) free(docdate);
	if (d == cm->DOR)
		d = Datedup(cm->DOR);

	return d;
}

static struct date *getDOC_prolongation(const CurrentMember cm[static 1],
		int k)
{
	unsigned addyear = 0;
	struct date *d = 0, *Ndate = 0, *docdate = 0;

	addyear = (cm->DOC[k]->month >= cm->DOC[1]->month) ? 1 : 0;
	Ndate = newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1);
	docdate = newDate(0, cm->DOC[k]->year + addyear, cm->DOC[1]->month, 1);

	if (0 == Ndate || 0 == docdate) die("invalid date");

	d = MINDATE2(Ndate, docdate);

	if (d != Ndate) free(Ndate);
	if (d != docdate) free(docdate);

	return d;
}

static void prolongate(CurrentMember cm[static 1], int k)
{
	for (int i = 0; i < EREE_AMOUNT; i++) {
		cm->PREMIUM[i][MAXGEN-1][k] = gensum(cm->PREMIUM, i, k);
		cm->CAPDTH[i][MAXGEN-1][k] = gensum(cm->CAPDTH, i, k);
		for (int l = 0; l < METHOD_AMOUNT; l++) {
			cm->RES[l][i][MAXGEN-1][k] =
				gensum(cm->RES[l], i, k);
			cm->RESPS[l][i][MAXGEN-1][k] =
				gensum(cm->RESPS[l], i, k); 
		}
		cm->DELTACAP[i][k] = 0;
		for (int j = 0; j < MAXGEN-1; j++) {
			cm->PREMIUM[i][j][k] = 0;
			cm->CAPDTH[i][j][k] = 0;
			for (int l = 0; l < METHOD_AMOUNT; l++) {
				cm->RES[l][i][j][k] = 0;
				cm->RESPS[l][i][j][k] = 0;
			} 
		}
	}

}