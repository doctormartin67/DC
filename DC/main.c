#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "DCProgram.h"
#include "actuarialfunctions.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Syntax: main \"Excel file\"\n");
    exit(0);
  }
    
  XLfile xl;
  DataSet ds;

  // These functions set all the necessary values of all the variables needed for the calculations.
  setXLvals(&xl, argv[1]);
  setDSvals(&xl, &ds);
  setCMvals(&ds);

  // Here the loop of all affiliates will start.
  currrun = runNewRF; // This needs updating when I start with reconciliation runs!!
  CurrentMember *cm = ds.cm;
  setassumptions(cm); // This defines the assumptions

  //***INITIALISE VARIABLES (k = 0)***
  //-  Dates and age  -
  *cm->DOC = cm->DOS;
  *cm->age = (*cm->DOC)->year - cm->DOB->year +
    (double)((*cm->DOC)->month - cm->DOB->month - 1)/12;
  *cm->nDOE = (*cm->DOC)->year - cm->DOE->year +
    (double)((*cm->DOC)->month - cm->DOE->month - (cm->DOE->day == 1 ? 0 : 1))/12;
  *cm->nDOA = (*cm->DOC)->year - cm->DOA->year +
    (double)((*cm->DOC)->month - cm->DOA->month - (cm->DOA->day == 1 ? 0 : 1))/12;

  cm->kPx[1] = 1;
  
  //-  Premium  -
  for (int EREE = 0; EREE < 2; EREE++) {
    *cm->PREMIUM[EREE][MAXGEN-1] = (EREE == ER ? calcA(cm, 0) : calcC(cm, 0));
    for (int j = 0; j < MAXGEN-1; j++) {
      *cm->PREMIUM[EREE][MAXGEN-1] =
	max(2, 0.0, *cm->PREMIUM[EREE][MAXGEN-1] - *cm->PREMIUM[EREE][j]);
    }
  }
  //***END INITIALISATION***  
  //---BEGIN LOOP---
  // This loops through all the years of an affiliate and makes the calculations.
  for (int k = 1; k < MAXPROJ; k++) {
    if (k > 1)
      cm->DOC[k] = minDate(3,
			   newDate(0, cm->DOB->year + NRA(cm, k-1), cm->DOB->month + 1, 1),
			   newDate(0, cm->DOC[k-1]->year + 1, cm->DOC[k-1]->month, 1),
			   cm->DOR);
    cm->DOC[k+1] = minDate(3,
			   newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1),
			   newDate(0, cm->DOC[k]->year + 1, cm->DOC[k]->month, 1),
			   cm->DOR);

    //***PROLONGATION***
    // Determining the DOC
    if (k == MAXPROJBEFOREPROL)
      cm->DOC[k+1] = minDate(2,
			     newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1),
			     newDate(0, cm->DOC[k]->year +
				     ((cm->DOC[k]->month >= cm->DOC[1]->month) ? 1 : 0),
				     cm->DOC[1]->month, 1));
    if (k > MAXPROJBEFOREPROL) {
      cm->DOC[k] = minDate(2,
			   newDate(0, cm->DOB->year + NRA(cm, k-1), cm->DOB->month + 1, 1),
			   newDate(0, cm->DOC[k-1]->year +
				   ((cm->DOC[k-1]->month >= cm->DOC[1]->month) ? 1 : 0),
				   cm->DOC[1]->month, 1));
      cm->DOC[k+1] = minDate(2,
			     newDate(0, cm->DOB->year + NRA(cm, k), cm->DOB->month + 1, 1),
			     newDate(0, cm->DOC[k]->year +
				     ((cm->DOC[k]->month >= cm->DOC[1]->month) ? 1 : 0),
				     cm->DOC[1]->month, 1));
      // Set Prolongation values
      // All values get put in last generation (MAXGEN)
      if (k == MAXPROJBEFOREPROL + 1) {
	int l; // Method
	for (int EREE = 0; EREE < 2; EREE++) {
	  cm->PREMIUM[EREE][MAXGEN-1][k-1] = gensum(cm->PREMIUM, EREE, k-1);
	  cm->CAPDTH[EREE][MAXGEN-1][k-1] = gensum(cm->CAPDTH, EREE, k-1);
	  for (l = 0; l < TUCPS_1 + 1; l++) {
	    cm->RES[l][EREE][MAXGEN-1][k-1] = gensum(cm->RES[l], EREE, k-1);
	    cm->RESPS[l][EREE][MAXGEN-1][k-1] = gensum(cm->RESPS[l], EREE, k-1);	    
	  }
	  cm->DELTACAP[EREE][k-1] = 0;
	  for (int j = 0; j < MAXGEN-1; j++) {
	    cm->PREMIUM[EREE][j][k-1] = 0;
	    cm->CAPDTH[EREE][j][k-1] = 0;
	    for (l = 0; l < TUCPS_1 + 1; l++) {
	      cm->RES[l][EREE][j][k-1] = 0;
	      cm->RESPS[l][EREE][j][k-1] = 0;
	    } 
	  }
	}
      }
    }
    
    //***END PROLONGATION***
    
    cm->age[k] = cm->DOC[k]->year - cm->DOB->year +
      (double)(cm->DOC[k]->month - cm->DOB->month - 1)/12;
    cm->age[k+1] = cm->DOC[k+1]->year - cm->DOB->year +
      (double)(cm->DOC[k+1]->month - cm->DOB->month - 1)/12;
    cm->nDOA[k] = cm->DOC[k]->year - cm->DOA->year +
      (double)(cm->DOC[k]->month - cm->DOA->month - (cm->DOA->day == 1 ? 0 : 1))/12;
    cm->nDOE[k] = cm->DOC[k]->year - cm->DOE->year +
      (double)(cm->DOC[k]->month - cm->DOE->month - (cm->DOE->day == 1 ? 0 : 1))/12;

    cm->sal[k] = cm->sal[k-1] *
      pow((1 + salaryscale(cm, k)),
	  cm->DOC[k]->year - cm->DOC[k-1]->year + (k == 1 ? ass.incrSalk1 : 0));

    //***RESERVES EVOLUTION***
    // Employer-Employee
    for (int EREE = ER; EREE < EE + 1; EREE++) {
      for (int gen = 0; gen < MAXGEN; gen++) {
	/* here all the functions will use the k-1 value to calculate the kth value*/
	cm->DELTACAP[EREE][k] = cm->DELTACAP[EREE][k-1];
	evolCAPDTH(cm, EREE, gen, k-1);
	evolRES(cm, EREE, gen, k-1); // This will also set the CAP values
      }
    }

    //***PREMIUMS EVOLUTION***
    //-  Retirement Premium
    // These variables are needed for Risk Premiums
    double Ax1;
    double ax;
    double axcost;
    for (int EREE = 0; EREE < EE + 1; EREE++) {
      for (int j = 0; j < MAXGEN; j++) {
	cm->PREMIUM[EREE][j][k] = (EREE == ER ? calcA(cm, k) : calcC(cm, k));
	for (int l = 0; l < j; l++) {
	  cm->PREMIUM[EREE][j][k] = cm->PREMIUM[EREE][j][k] - cm->PREMIUM[EREE][l][k];
	}
	cm->PREMIUM[EREE][j][k] =
	  (j < MAXGEN-1 ? min(2, cm->PREMIUM[EREE][j][k-1], cm->PREMIUM[EREE][j][k]) :
	   cm->PREMIUM[EREE][j][k]);
	//-  Risk Premium (only for MIXED)  -
	if (cm->age[k] == cm->age[k-1] || (cm->tariff != MIXED))
	  cm->RP[EREE][j][k-1] = 0;
	else {
	  Ax1 = Ax1n(tff.ltINS[EREE][j].lt, cm->TAUX[EREE][j], tff.costRES,
		     cm->age[k-1], cm->age[k], 0);
	  ax = axn(tff.ltINS[EREE][j].lt, cm->TAUX[EREE][j], tff.costRES,
		   tff.prepost, tff.term, cm->age[k-1], cm->age[k], 0);
	  axcost = axn(tff.ltINS[EREE][j].lt, cm->TAUX[EREE][j], tff.costRES,
		       0, 1, cm->age[k-1], cm->age[k], 0);
	  cm->RP[EREE][j][k-1] =
	    max(2, 0.0, (cm->CAP[EREE][j][k-1]/cm->X10 - cm->RES[PUC][EREE][j][k-1]) * Ax1/ax) +
	    cm->CAP[EREE][j][k-1]/cm->X10 * tff.costKO * axcost;
	}			     
      }
    }

    //***ART24 EVOLUTION***
    // the function will use the k-1 value to calculate the kth value
    evolART24(cm, k-1);

    // The following variables get updated each loop
    double ART24TOT[TUCPS_1 + 1];
    double RESTOT[TUCPS_1 + 1];
    double REDCAPTOT[TUCPS_1 + 1];
    
    for (int j = 0; j < TUCPS_1 + 1; j++) {
      ART24TOT[j] = cm->ART24[j][ER][ART24GEN1][k] + cm->ART24[j][ER][ART24GEN2][k] +
	cm->ART24[j][EE][ART24GEN1][k] + cm->ART24[j][EE][ART24GEN2][k];
      RESTOT[j] = gensum(cm->RES[j], ER, k) + gensum(cm->RES[j], EE, k) +
	gensum(cm->RESPS[j], ER, k) + gensum(cm->RESPS[j], EE, k);
      REDCAPTOT[j] = gensum(cm->REDCAP[j], ER, k) + gensum(cm->REDCAP[j], EE, k);
    }

    //***DBO CALCULATION - RETIREMENT***
    // Funding factors
    if (!(cm->status & ACT) || gensum(cm->PREMIUM, ER, 1) + gensum(cm->PREMIUM, EE, 1) == 0) {
      cm->FF[k] = 1;
      cm->FFSC[k] = 0;
    }
    else {
      cm->FF[k] = cm->nDOA[1] / (cm->nDOA[k] == 0 ? 1 : cm->nDOA[k]);
      cm->FFSC[k] = (k == 1 ? 0 : (cm->age[2] - cm->age[1]) /
		     (cm->nDOA[k] == 0 ? 1 : cm->nDOA[k]));
      
    }
    // Turnover rates
    cm->wxdef[k] = wxdef(cm, k) * (cm->age[k+1] - cm->age[k]) * ass.TRM_PercDef;
    cm->wximm[k] = wxdef(cm, k) * (cm->age[k+1] - cm->age[k]) * (1 - ass.TRM_PercDef);

    // Life and Death rates
    cm->qx[k] = 1 - npx((cm->status & MALE ? lifetables[LXMR] : lifetables[LXFR]),
		    cm->age[k], cm->age[k+1], ass.agecorr);
    cm->retx[k] = retx(cm, k) * (k > 1 && cm->age[k] == cm->age[k-1] ? 0 : 1);
    cm->nPk[k] = npx((cm->status & MALE ? lifetables[LXMR] : lifetables[LXFR]),
		     cm->age[k], NRA(cm, k), ass.agecorr);

    // Financial variables (discount rate)
    cm->vk[k] = pow(1 + ass.DR, -(cm->age[k] - cm->age[1]));
    cm->vn[k] = pow(1 + ass.DR, -(NRA(cm, k) - cm->age[1]));    
    cm->vk113[k] = pow(1 + ass.DR113, -(cm->age[k] - cm->age[1]));
    cm->vn113[k] = pow(1 + ass.DR113, -(NRA(cm, k) - cm->age[1]));    

    // Calculation
    // DBO PUC
    cm->DBORET[PUC][PAR115][k] =
      max(2, ART24TOT[PUC] * cm->FF[k], REDCAPTOT[TUC]) *
      cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] +
      max(2, ART24TOT[PUC] * cm->FF[k], RESTOT[TUC]) *
      (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];

    cm->DBORET[PUC][MATHRES][k] =
      ART24TOT[PUC] * cm->FF[k] * cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] +
      ART24TOT[PUC] * cm->FF[k] * (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];

    cm->DBORET[PUC][PAR113][k] = cm->DBORET[PUC][PAR115][k];

    // DBO TUC
    cm->DBORET[TUC][PAR115][k] =
      max(2, ART24TOT[TUC], REDCAPTOT[TUC]) * cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] +
      max(2, ART24TOT[TUC], RESTOT[TUC]) * (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];

    cm->DBORET[TUC][MATHRES][k] =
      ART24TOT[TUC] * cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] +
      ART24TOT[TUC] * (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];

    cm->DBORET[TUC][PAR113][k] = cm->DBORET[TUC][PAR115][k];

    // NC PUC
    cm->NCRET[PUC][PAR115][k] =
      ART24TOT[PUC] * cm->FFSC[k] * cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] +
      ART24TOT[PUC] * cm->FFSC[k] * (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];

    cm->NCRET[PUC][PAR113][k] = cm->NCRET[PUC][MATHRES][k] = cm->NCRET[PUC][PAR115][k];
    
    // NC TUC
    cm->NCRET[TUC][PAR115][k] =
      (ART24TOT[TUCPS_1] - ART24TOT[TUC]) * cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] +
      (ART24TOT[TUCPS_1] - ART24TOT[TUC]) * (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];

    cm->NCRET[TUC][PAR113][k] = cm->NCRET[TUC][MATHRES][k] = cm->NCRET[TUC][PAR115][k];

    // IC NC PUC
    cm->ICNCRET[PUC][PAR115][k] =
      ART24TOT[PUC] * cm->FFSC[k] * cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] * ass.DR +
      ART24TOT[PUC] * cm->FFSC[k] * (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k] * ass.DR;

    cm->ICNCRET[PUC][PAR113][k] = cm->ICNCRET[PUC][MATHRES][k] = cm->ICNCRET[PUC][PAR115][k];
    
    // IC NC TUC
    cm->ICNCRET[TUC][PAR115][k] =
      (ART24TOT[TUCPS_1] - ART24TOT[TUC]) *
      cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] * ass.DR+
      (ART24TOT[TUCPS_1] - ART24TOT[TUC]) *
      (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k] * ass.DR;

    cm->ICNCRET[TUC][PAR113][k] = cm->ICNCRET[TUC][MATHRES][k] = cm->ICNCRET[TUC][PAR115][k];

    // Plan Assets
    cm->assets[PAR115][k] =
      REDCAPTOT[TUC] * cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn[k] +
      RESTOT[TUC] * (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk[k];
    cm->assets[PAR113][k] =
      REDCAPTOT[TUC] * cm->wxdef[k] * cm->kPx[k] * cm->nPk[k] * cm->vn113[k] +
      RESTOT[TUC] * (cm->wximm[k] + cm->retx[k]) * cm->kPx[k] * cm->vk113[k];

    // kPx is defined after first loop
    if (k+1 < MAXPROJ)
      cm->kPx[k+1] =
	cm->kPx[k] * (1 - cm->qx[k]) * (1 - cm->wxdef[k] - cm->wximm[k]) * (1 - cm->retx[k]);
  }     
  // create excel file to print results
  printresults(&ds);
  return 0;
}
