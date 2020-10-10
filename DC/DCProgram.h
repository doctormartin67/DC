#ifndef DCPROGRAM
#define DCPROGRAM

#include "libraryheader.h"
#include "hashtable.h"

typedef struct dataset {
  int keyrow; /* find the row in the excel file where 
		 the keys are to use in the hashtable */
  char keycolumn[3];
  char datasheet[256]; // This is the sheet where the data lies
  XLfile *xl;
  Hashtable ***Data;
  int membercnt;
		 
} DataSet;

void setDSvals(XLfile *xl, DataSet *ds);

/* This function will allocate memory based on membercnt for the underlying
   Hashtable used for the data.*/
void createData(DataSet *ds);

/* used to find the row and sheet where the keys lie for the data to be used
   for calculations. If the word KEY isn't found in the data then
   1 is returned */
void setkey(DataSet *ds);
void countMembers(DataSet *ds);
#endif
