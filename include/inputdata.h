#ifndef INPUTDATA
#define INPUTDATA

/* 
 * The program will expect the following columns to be present in the data.
 * If the column is not present, the program will the set values for the column
 * to zero and not use it
 */
typedef enum {KEY, NOREGLEMENT, NAAM, CONTRACT, STATUS, ACTIVECONTRACT, SEX,
	MS, DOB, DOE, DOL, DOS, DOA, DOR, CATEGORIE, SAL, PG, PT, NORMRA, ENF,
	TARIEF, ART24, PREMIUM, CAP, CAPPS, CAPDTH, RES,
	RESPS, CAPRED, TAUX, DELTA_CAP, X10, RP,
	KEYS_AMOUNT = 1024
} DataColumn;

extern const char *const colnames[KEYS_AMOUNT];

#endif
