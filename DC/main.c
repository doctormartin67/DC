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
	/* here all the functions will use the k-1 value to calculate the kth value and  */
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
	//-  Risk Premium  -
	if (cm->age[k] == cm->age[k-1])
	  cm->RP[EREE][j][k-1] = 0;
	else {
	  Ax1 = Ax1n(tff.ltINS, cm->TAUX[EREE][j], tff.costRES, cm->age[k-1], cm->age[k], 0);
	  ax = axn(tff.ltINS, cm->TAUX[EREE][j], tff.costRES,
		   tff.prepost, tff.term, cm->age[k-1], cm->age[k], 0);
	  axcost = axn(tff.ltINS, cm->TAUX[EREE][j], tff.costRES,
		       0, 1, cm->age[k-1], cm->age[k], 0);
	  cm->RP[EREE][j][k-1] =
	    max(2, 0.0, (cm->CAP[EREE][j][k-1]/cm->X10 - cm->RES[PUC][EREE][j][k-1]) * Ax1/ax) +
	    cm->CAP[EREE][j][k-1]/cm->X10 * tff.costKO * axcost;
	}			     
      }
    }

    //***ART24 EVOLUTION***
    
  }
  // create excel file to print results
  printresults(&ds);
  return 0;
}
