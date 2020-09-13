#ifndef DCPROGRAM
#define DCPROGRAM

#include "hashtable.h"

typedef struct currentMember {
  int keyrow; /* find the row in the excel file where 
		 the keys are to use in the hashtable */
  Hashtable **Data;
		 
} CurrentMember;
