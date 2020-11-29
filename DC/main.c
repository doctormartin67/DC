#include <stdio.h>
#include <stdlib.h>
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
  for (int k = 0; k < MAXPROJ; k++) {
    if (k > 1)
      cm->DOC[k] = minDate(3,
			   newDate(0, cm->DOB->year + NRA(cm), cm->DOB->month + 1, 1),
			   newDate(0, cm->DOC[k-1]->year + 1, cm->DOC[k-1]->month, 1),
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
      if (k == MAXPROJBEFOREPROL + 1) {
	;// to be continued: row 505 in access
      }
    }

    //***END PROLONGATION***
    
  }
  // create excel file to print results
  printresults(&ds);
  return 0;
}
