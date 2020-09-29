#ifndef DCPROGRAM
#define DCPROGRAM

#include "libraryheader.h"
#include "hashtable.h"

typedef struct currentMember {
  int keyrow; /* find the row in the excel file where 
		 the keys are to use in the hashtable */
  char keycolumn[3];
  char datasheet[256]; // This is the sheet where the data lies
  Hashtable **Data;
		 
} CurrentMember;

void setCMvals(XLfile *xl, CurrentMember *cm);

/* used to find the row and sheet where the keys lie for the data to be used
   for calculations. If the word KEY isn't found in the data then
   1 is returned */
void setkey(XLfile *xl, CurrentMember *cm);
#endif
