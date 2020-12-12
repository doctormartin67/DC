#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "libraryheader.h"
#include "hashtable.h"
#include "DCProgram.h"

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
  setassumptions(ds.cm);

  //---BEGIN LOOP---
  // This loops through all the years of an affiliate and makes the calculations.
  CurrentMember *cm = ds.cm;
  // start at k = 1 because k = 0 should have been initialised with setCMvals(&ds)
  for (int k = 1; k < MAXPROJ; k++) {
    if (k > 1)
      cm->DOC[k] = minDate(3,
			   newDate(0, cm->DOB->year + NRA(cm), cm->DOB->month + 1, 1),
			   newDate(0, cm->DOC[k-1]->year + 1, cm->DOC[k-1]->month, 1),
			   cm->DOR);
    cm->DOC[k+1] = minDate(3,
			   newDate(0, cm->DOB->year + NRA(cm), cm->DOB->month + 1, 1),
			   newDate(0, cm->DOC[k]->year + 1, cm->DOC[k]->month, 1),
			   cm->DOR);
    

    //***PROLONGATION***
    // Determining the DOC
    if (k == MAXPROJBEFOREPROL)
      cm->DOC[k+1] = minDate(2,
			     newDate(0, cm->DOB->year + NRA(cm), cm->DOB->month + 1, 1),
			     newDate(0, cm->DOC[k]->year +
				     ((cm->DOC[k]->month >= cm->DOC[1]->month) ? 1 : 0),
				     cm->DOC[1]->month, 1));
    if (k > MAXPROJBEFOREPROL) {
      cm->DOC[k] = minDate(2,
			   newDate(0, cm->DOB->year + NRA(cm), cm->DOB->month + 1, 1),
			   newDate(0, cm->DOC[k-1]->year +
				   ((cm->DOC[k-1]->month >= cm->DOC[1]->month) ? 1 : 0),
				   cm->DOC[1]->month, 1));
      cm->DOC[k+1] = minDate(2,
			     newDate(0, cm->DOB->year + NRA(cm), cm->DOB->month + 1, 1),
			     newDate(0, cm->DOC[k]->year +
				     ((cm->DOC[k]->month >= cm->DOC[1]->month) ? 1 : 0),
				     cm->DOC[1]->month, 1));
      // Set Prolongation values
      // All values get put in last generation (MAXGEN)
      if (k == MAXPROJBEFOREPROL + 1) {
	int j;
	for (int EREE = 0; EREE < EE + 1; EREE++) {
	  cm->PREMIUM[EREE][MAXGEN-1][k-1] = gensum(cm->PREMIUM, EREE, k-1);
	  cm->CAPDTH[EREE][MAXGEN-1][k-1] = gensum(cm->CAPDTH, EREE, k-1);
	  for (j = 0; j < TUCPS_1 + 1; j++) {
	    cm->RES[j][EREE][MAXGEN-1][k-1] = gensum(cm->RES[j], EREE, k-1);
	    cm->RESPS[j][EREE][MAXGEN-1][k-1] = gensum(cm->RESPS[j], EREE, k-1);	    
	  }
	  cm->DELTACAP[EREE][k-1] = 0;
	  for (j = 0; j < MAXGEN-1; j++) {
	    cm->PREMIUM[EREE][j][k-1] = 0;
	    cm->CAPDTH[EREE][j][k-1] = 0;
	    for (int l = 0; j < TUCPS_1 + 1; j++) {
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

    /*    cm->sal[k] = cm->sal[k-1] *
      pow((1 + salaryscale(cm, k)), cm->DOC[k]->year - cm->DOC[k-1]->year);
    */
  }
  // create excel file to print results
  printresults(&ds);
  return 0;
}
