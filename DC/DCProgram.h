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

//-  method BITS -
#define mIAS 0x1 // used to set IAS bit on (off means FAS)
#define mDTH 0x2 // used to set DTH bit on if death is evaluated
#define mTUC 0x4 // used to set TUC bit on (off means PUC)
#define mmaxERContr 0x8 /* used to set bit on if we take the maximum between 
			   the contributions and the service cost (never used in
			   case we have PUC methodology) */
#define mmaxPUCTUC 0x10 // used to set bit on if we take maximum of PUC and TUC
#define mRES 0x20 // used to set bit on in case assets are mathematical reserves
#define mPAR115 0x40 // used to set bit on in case assets are paragraph 115
// if neither mRES, mPAR115 bits are on, then we take paragraph 113

//-  extra BITS  -
#define INCSAL 01; // put this bit on when we increase the salary in the first line
#define CCRA 02; // put this bit on if this is a prepensioner

//---Define constants---
// 65 years to calculate of one affiliate
// 50 years up to NRA, afterwards we prolongate to RA assumption
enum {MAXPROJ = 65, MAXPROJBEFOREPROL = 50};
enum {MAXGEN = 8}; // amount of generations of insurer
enum {ER, EE};// Employer index, Employer index
// article 24 index (3,75% for employee contributions and 3,25% for employee)
// article 24 index (1,75%)
enum {ART24GEN1, ART24GEN2};
#define ART24admincost 0.05 // Maximum admin cost that may be applied to Employer contribution
enum inscomb {UKMS, UKZT, UKMT, MIXED};
// projected unit credit with future premiums
// projected unit credit without future premiums
/* projected unit credit with future premiums,
   one year later (for service cost)*/
enum {PUC, TUC, TUCPS_1};
// Assets $115
// Assets Mathematical Reserves
// Assets $113
enum assets {PAR115, MATHRES, PAR113};
// PBO Cashflows
// TBO Cashflows
enum cashflows {PBO, TBO};

static const double ART24TAUX[2][2] = {{0.0325, 0.0175}, {0.0375, 0.0175}};
/* Current guarenteed rates that the employers need to guarentee on the 
   reserves of their employees by Belgian law (Employer-Employee, generation)*/

typedef struct {
    Hashtable *Data; //Data for an affiliate is in the form of a hashtable

    //---Variable Declarations---  
    char *key; // KEY in data
    char *regl; // REGLEMENT
    char *name; // NAME
    char *contract; // CONTRACT number
    unsigned short status; // 0000 0000 0000 0111 means single male active member and active contract
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
    double NRA; // normal retirement age
    unsigned short kids; // amount of kids
    unsigned short tariff; // UKMS, UKZT, UKMT, MIXED
    double KO; // death lump sum (kapitaal overlijden)
    double annINV; // annuity in case of invalidity
    double contrINV; // contribution for invalidity insurance
    double *ART24[TUCPS_1 + 1][2][2]; /* WAP: art24 reserves 
					 (Method, Exployer-Employee, generation, loops)*/
    // Currently there are 2 generations for article 24 and 3 methods needed
    double *CAP[2][MAXGEN]; // Pension lump sum (Employer-Employee, generations, loops)
    double *CAPPS[2][MAXGEN]; /* Pension lump sum profit sharing 
				 (Employer-Employee, generations, loops)*/
    double *REDCAP[TUCPS_1 + 1][2][MAXGEN]; /* Reduced lump sum 
					       (Employer-Employee, generations, Method, loops)*/
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
    double *RP[2][MAXGEN]; // Risk Premium
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
			     means prepensioner whose salary we increase at k = -1 */

    //---Variable definitions---    
    double *age; // age of affiliate
    double *nDOE; // years since date of entry
    double *nDOA; // years since date of affiliation

    //---Variables that are used for DBO calculation---
    // For k = 0 these will all be undefined!!
    double *FF; // Funding Factor
    double *FFSC; // Funding Factor Service Cost
    double *qx; // Chance to die within 1 year
    double *wxdef; // Turnover rate within 1 year (deferred payment)
    double *wximm; // Turnover rate within 1 year (immediate payment)
    double *retx; // Chance to retire within 1 year (usually 100% at 65)
    double *nPk; // Chance to live from now until retirement
    double *kPx; // Chance to live from the start until now
    double *vk; // 1/(1+DR)^k with DR = discount rate
    double *vn; // 1/(1+DR)^n with DR = discount rate
    double *vk113; // 1/(1+DR)^k with DR = discount rate according to IAS19 $113
    double *vn113; // 1/(1+DR)^n with DR = discount rate according to IAS19 $113

    double *DBORET[2][3]; // DBO Retirement (PUC - TUC, Method Assets, loops)
    double *NCRET[2][3]; // Normal Cost Retirement (PUC - TUC, Method Assets, loops)
    double *ICNCRET[2][3]; /* Interest Cost on Normal Cost Retirement 
			      (PUC - TUC, Method Assets, loops) */
    double *assets[3]; // Plan Assets ($115, Mathematical Reserves, $113)
    double *AFSL;

    //---DBO DTH---
    double *CAPDTHRESPart; // Death Lump Sum Reserves Part
    double *CAPDTHRiskPart; // Death Lump Sum Risk Part
    double *DBODTHRESPart; // DBO Death Reserves Part
    double *DBODTHRiskPart; // DBO Death Risk Part
    double *NCDTHRESPart; // NC Death Reserves Part
    double *NCDTHRiskPart; // NC Death Risk Part
    double *ICNCDTHRESPart; // Interest Cost on Normal Cost Death Reserves Part
    double *ICNCDTHRiskPart; // Interest Cost on Normal Cost Death Risk Part

    //---CASHFLOWS---
    double *EBP[2][3][2]; /* Expected Benefits Paid (PUC - TUC, Method Assets,
			     PBO - TBO, loops) */
    double *PBONCCF[2][3]; // PBO Normal Cost Cashflows
    double *EBPDTH[2]; // Expected Benefits Paid Death
    double *PBODTHNCCF; // PBO Death Normal Cost Cashflows
} CurrentMember;

//---Useful functions for CurrentMembers---
double gensum(double *amount[][MAXGEN], unsigned short EREE, int loop);

typedef struct {
    int keyrow; /* find the row in the excel file where 
		   the keys are to use in the hashtable */
    char keycolumn[4];
    char **keys; // This points to the array of keys in excel
    char datasheet[256]; // This is the sheet where the data lies
    XLfile *xl;
    Hashtable **Data; // This will be set using createData function below
    int membercnt;
    CurrentMember *cm; // This is a pointer to the affiliates	 
} DataSet;

//---Assumptions declarations---
typedef struct {
    Date *DOC; /* This is DOC[1] which is the start of the run through affiliates.
		  DOC[0] is date of situation.*/
    double infl; // Inflation
    double DR; // Discount Rate
    double DR113; // Discount Rate under $113 of IAS19  
    short agecorr; // Age Correction
    double (*SS)(CurrentMember *cm, int k); // Salary Scale
    double (*calcA)(CurrentMember *cm, int k); // Formula for Employer retirement contribution
    double (*calcC)(CurrentMember *cm, int k); // Formula for Employee retirement contribution  
    double (*calcDTH)(CurrentMember *cm, int k); // Formula for Capital Death
    double (*NRA)(CurrentMember *cm, int k);
    double (*wxdef)(CurrentMember *cm, int k); // Turnover rate with deferred payment
    double (*retx)(CurrentMember *cm, int k); // Turnover rate with deferred payment

    //Assumptions that usually won't change from year to year
    unsigned short incrSalk0; // determine whether sal gets increased at k = 0 
    unsigned short incrSalk1; // determine whether sal gets increased at k = 1 
    double TRM_PercDef; /* Percentage of deferred members that will keep their
			   reserves with the current employer at termination (usually equals 1)*/
    unsigned short method; // Methodology, we use bits here
    double taxes; /* used for the Service cost because the taxes shouldn't be included
		     in the admin cost (usually equal to 0.1326 */
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
double calcA(CurrentMember *cm, int k);
double calcC(CurrentMember *cm, int k);
double calcDTH(CurrentMember *cm, int k);
double NRA(CurrentMember *cm, int k);
double wxdef(CurrentMember *cm, int k);
double retx(CurrentMember *cm, int k);

//---Tariff Structure---

typedef struct {
    unsigned int lt; // there is an array of strings containing the names of lifetables. I reference the array elements by using the index defined with an enum lifetables.
    double i; // Insurance rate (This changes for prolongation table for example)
} LifeTable;

typedef struct {
    LifeTable ltINS[2][MAXGEN]; // Life Table Insurer
    LifeTable ltAfterTRM[2][MAXGEN]; // Life Table after termination
    LifeTable ltProlong[2]; // Life Table Prolongation (i = last generation of insurer)
    LifeTable ltProlongAfterTRM[2]; // Life Table Prolongation (i = last generation of insurer)  
    double costRES; // Cost on reserves
    double WDDTH; // Profit sharing death (winstdeelname)
    double costKO; // cost on Death lump sum (kapitaal overlijden)
    double admincost; // Administration cost
    double MIXEDPS;
    int prepost; // prenumerando-postnumerando
    int term; // term of payments
} Tariff;

Tariff tff; // Tariff structure

//---Setter declarations---
void setDSvals(XLfile *xl, DataSet *ds);
void setCMvals(DataSet *ds);
char *getcmval(CurrentMember *cm, char *value);
void allocvar(CurrentMember *cm, double *var[][MAXGEN], char *s);
/* This function will allocate memory based on membercnt for the underlying
   Hashtable used for the data.*/
void createData(DataSet *ds);

/* used to find the row and sheet where the keys lie for the data to be used
   for calculations. If the word KEY isn't found in the data then
   1 is returned */
void setkey(DataSet *ds);
void countMembers(DataSet *ds);
int printresults(DataSet *ds, int tc); // tc = test case

#endif
