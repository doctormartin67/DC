#ifndef DCPROGRAM
#define DCPROGRAM

#include "libraryheader.h"
#include "hashtable.h"

//---Define BIT constants---
//-  status BITS  -
#define ACT 0x1 // used to set ACT bit on if active member
#define ACTCON 0x2// used to set ACTCON bit on if it is an active contract
#define MALE 0x4 // used to set MALE bit on if it is a male
#define MARRIED 0x8 // used to set MARRIED bit on if it is married

//-  extra BITS  -
#define INCSAL 01; // put this bit on when we increase the salary in the first line
#define CCRA 02; // put this bit on if this is a prepensioner

//---Define constants---
#define MAXPROJ 65 // years to calculate of one affiliate
#define MAXPROJBEFOREPROL 50 // years up to NRA, afterwards we prolongate to RA assumption
#define MAXGEN 8 // amount of generations of insurer
#define ER 0 // Employer index
#define EE 1 // Employee index
#define ART24GEN1 0 // article 24 index (3,75% for employee contributions and 3,25% for employee)
#define ART24GEN2 1 // article 24 index (1,75%)
#define UKMS 0
#define UKZT 1
#define UKMT 2
#define MIXED 3
#define PUC 0 // projected unit credit with future premiums
#define TUC 1 // projected unit credit without future premiums
#define TUCPS_1 2 /* projected unit credit with future premiums,
		       one year later (for service cost)*/

typedef struct currentmember {
  Hashtable **Data; //Data for an affiliate is in the form of a hashtable

  //---Variable Declarations---  
  char *key; // KEY in data
  char *regl; // REGLEMENT
  char *name; // NAME
  char *contract; // CONTRACT number
  unsigned short status; // 0000 0000 0000 0111 means male active member and active contract
  Date *DOB; // date of birth
  Date *DOE; // date of entry
  Date *DOL; // date of leaving
  Date *DOS; // date of situation
  Date *DOA; // date of affiliation
  Date *DOR; // date of retirement
  Date **DOC; // date of calculation (will be an array)
  char *category; // for example blue collar, white collar, management, ...
  double *sal; // salary (array)
  double PG; // pensioengrondslag ( I have never needed this)
  double PT; // part time
  unsigned short NRA; // normal retirement age
  unsigned short kids; // amount of kids
  unsigned short tariff; // UKMS, UKZT, UKMT, MIXED
  double KO; // death lump sum (kapitaal overlijden)
  double annINV; // annuity in case of invalidity
  double contrINV; // contribution for invalidity insurance
  double *ART24[2][ART24GEN2 + 1][TUCPS_1 + 1]; /* WAP: art24 reserves 
						   (Exployer-Employee, generation, Method, loops)*/
  // Currently there are 2 generations for article 24 and 3 methods needed
  double *CAP[2][MAXGEN]; // Pension lump sum (Employer-Employee, generations, loops)
  double *CAPPS[2][MAXGEN]; /* Pension lump sum profit sharing 
  				       (Employer-Employee, generations, loops)*/
  double *REDCAP[2][MAXGEN]; // Reduced lump sum (Employer-Employee, generations, loops)
  double TAUX[2][MAXGEN]; /* return guarentee insurer
  				      (Employer-Employee, generations, loops)*/
  double *PREMIUM[2][MAXGEN]; // Contribution (Employer-Employee, generations, loops)
  double *RES[TUCPS_1 + 1][2][MAXGEN]; // Reserves (Employer-Employee, generations, Method, loops)
  double *RESPS[TUCPS_1 + 1][2][MAXGEN]; /* Profit Sharing Reserves 
					    (Employer-Employee, generations, Method, loops)*/
  double *DELTACAP[2]; // Delta Cap (AXA) (Employer-Employee, generations, loops)
  double X10; // MIXED combination
  double *CAPDTH[2][MAXGEN]; /* Death lump sum (used for UKMT)
  					(Employer-Employee, generations, loops)*/
  double CAO; // collectieve arbeidsovereenkomst
  char *ORU;
  char *CHOICEDTH; // Choice of death insurance
  char *CHOICEINVS; // Choice of invalide insurance sickness
  char *CHOICEINVW; // Choice of invalide insurance work
  double contrDTH; // Death contributions
  double percSALKO; // percentage of salary for death lump sum
  char *indexINV; // indexation for invalidity
  char *GRDGR;
  char *plan;
  double baranc; // baremic ancienity
  unsigned short extra; /* 0000 0000 0000 0011
			   means prepensioner whose salary we increase at k = -1*/
  
  //---Variable definitions---    
  double *age; // age of affiliate
  double *nDOE; // years since date of entry
  double *nDOA; // years since date of affiliation

  //---Variables that are used to distinguish between clients---
  
} CurrentMember;

//---Useful functions for CurrentMembers---
double gensum(double *amount[][MAXGEN], unsigned short EREE, int loop);

typedef struct dataset {
  int keyrow; /* find the row in the excel file where 
		 the keys are to use in the hashtable */
  char keycolumn[3];
  char **keys; // This points to the array of keys in excel
  char datasheet[256]; // This is the sheet where the data lies
  XLfile *xl;
  Hashtable ***Data; // This will be set using createData function below
  int membercnt;
  CurrentMember *cm; // This is a pointer to the affiliates	 
} DataSet;

//---Assumptions declarations---
typedef struct assumptions {
  Date *DOC; /* This is DOC[1] which is the start of the run through affiliates.
	       DOC[0] is date of situation.*/
  double DR; // Discount Rate
  double DR113; // Discount Rate under $113 of IAS19  
  short agecorr; // Age Correction
  double (*SS)(CurrentMember *cm, int k);
  double infl; // Inflation
  unsigned short (*NRA)(CurrentMember *cm, int k);
  double (*wxdef)(CurrentMember *cm, int k); // Turnover rate with deferred payment
  double (*wximm)(CurrentMember *cm, int k); // Turnover rate with immediate payment

  //Assumptions that usually won't change from year to year
  unsigned short incrSalk1; // determine whether sal gets increased at k = 1 
} Assumptions;

Assumptions ass; // Assumptions

//---Reconciliation declarations---
enum runs {runLY, runUpdateInflation, runUpdateDR, runRF,
	   runNewData, runNewMethodology, runNewMortality,
	   runNewTurnover, runNewNRA, runNewInflation, runNewSS,
	   runNewDR, runNewRF, runSensitivities};
enum sensruns {runsensDRminus = 21, runsensDRplus, runsensInflationminus,
	       runsensInflationplus, runsensSSminus, runsensSSplus,
	       runsensAgeCorrminus, runsensAgeCorrplus, runsensDuration};
unsigned short currrun; // Current run
void setassumptions(CurrentMember *cm);
double salaryscale(CurrentMember *cm, int k);
unsigned short NRA(CurrentMember *cm, int k);
double wxdef(CurrentMember *cm, int k);
double wximm(CurrentMember *cm, int k);

//---Tariff Structure---

typedef struct tariff {
  char *ltINS; // Life Table Insurer
  char *ltAfterTRM; // Life Table after termination
  double costRES; // Cost on reserves
  double WDDTH; // Profit sharing death (winstdeelname)
  double costKO; // cost on Death lump sum (kapitaal overlijden)
  double admincost; // Administration cost
  double MIXEDPS;
  int prepost; // prenumerando-postnumerando
  int term; // term of payments
} Tariff;

Tariff tff; // Tariff structure

static char *lifetables[6] =
  {"LXMR", "LXFR", "LXMK", "LXFK", "LXFK'", "Lxnihil"};
#define LXMR 0
#define LXFR 1
#define LXMK 2
#define LXFK 3
#define LXFKP 4
#define LXNIHIL 5

//---Setter declarations---
void setDSvals(XLfile *xl, DataSet *ds);
void setCMvals(DataSet *ds);
char *getcmval(CurrentMember *cm, char *value);

/* This function will allocate memory based on membercnt for the underlying
   Hashtable used for the data.*/
void createData(DataSet *ds);

/* used to find the row and sheet where the keys lie for the data to be used
   for calculations. If the word KEY isn't found in the data then
   1 is returned */
void setkey(DataSet *ds);
void countMembers(DataSet *ds);
int printresults(DataSet *ds);

#endif
