#ifndef DCPROGRAM
#define DCPROGRAM

#include "libraryheader.h"
#include "inputdata.h"
#include "validation.h"

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
#define mPAR115 0x30 // used to set bit on in case assets are paragraph 115
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

typedef double GenMatrix[MAXGEN][MAXPROJ]; 

//---Data Declarations---
typedef struct {
    /* --- Data --- */
    char fname[PATH_MAX];
    char sheetname[32];
    char keycell[11];
    
    /* --- Assumptions --- */
    char DOC[11];
    char DR[16];
    char agecorr[16];
    char infl[16];
    char TRM_PercDef[16];
    char DR113[16];
    char SI[16];
    char SS[BUFSIZ]; /* This could be a large text describing salary scale with select cases */
    /* turnover still needs to be added !!! */

    /* --- Methodology --- */
    gint standard;
    gint assets;
    gint paragraph;
    gint PUCTUC;
    gint cashflows;
    gint evaluateDTH;
} UserInput;

typedef struct {
    Hashtable *Data; //Data for an affiliate is in the form of a hashtable

    //---Variable Declarations---  
    char *key; // KEY in data
    char *regl; // REGLEMENT
    char *name; // NAME
    char *contract; // CONTRACT number
    unsigned short status; /* 0000 0000 0000 0111 means single male active member and 
			      active contract */
    Date *DOB; // date of birth
    Date *DOE; // date of entry
    Date *DOL; // date of leaving
    Date *DOS; // date of situation
    Date *DOA; // date of affiliation
    Date *DOR; // date of retirement
    Date **DOC; // date of calculation (will be an array)
    char *category; // for example blue collar, white collar, management, ...
    double sal[MAXPROJ]; // salary (array)
    double PG; // pensioengrondslag (I have never needed this)
    double PT; // part time
    double NRA; // normal retirement age
    unsigned short kids; // amount of kids
    unsigned short tariff; // UKMS, UKZT, UKMT, MIXED
    double KO; // death lump sum (kapitaal overlijden)
    double annINV; // annuity in case of invalidity
    double contrINV; // contribution for invalidity insurance
    double ART24[TUCPS_1 + 1][2][2][MAXPROJ]; /* WAP: art24 reserves 
					 (Method, Exployer-Employee, generation, loops)*/
    // Currently there are 2 generations for article 24 and 3 methods needed
    GenMatrix CAP[2]; // Pension lump sum (Employer-Employee, generations, loops)
    GenMatrix CAPPS[2]; /* Pension lump sum profit sharing 
				 (Employer-Employee, generations, loops)*/
    GenMatrix REDCAP[TUCPS_1 + 1][2]; /* Reduced lump sum 
					       (Employer-Employee, generations, Method, loops)*/
    double TAUX[2][MAXGEN]; /* return guarentee insurer
			       (Employer-Employee, generations, loops)*/
    GenMatrix PREMIUM[2]; // Contribution (Employer-Employee, generations, loops)
    GenMatrix RES[TUCPS_1 + 1][2]; // Reserves (Employer-Employee, generations, Method, loops)
    GenMatrix RESPS[TUCPS_1 + 1][2]; /* Profit Sharing Reserves 
					      (Employer-Employee, generations, Method, loops)*/
    double DELTACAP[2][MAXPROJ]; // Delta Cap (AXA) (Employer-Employee, generations, loops)
    double X10; // MIXED combination
    GenMatrix CAPDTH[2]; /* Death lump sum (used for UKMT)
				  (Employer-Employee, generations, loops)*/
    GenMatrix RP[2]; // Risk Premium
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
    double age[MAXPROJ + 1]; // age of affiliate
    double nDOE[MAXPROJ]; // years since date of entry
    double nDOA[MAXPROJ]; // years since date of affiliation

    //---Variables that are used for DBO calculation---
    // For k = 0 these will all be undefined!!
    double FF[MAXPROJ]; // Funding Factor
    double FFSC[MAXPROJ]; // Funding Factor Service Cost
    double qx[MAXPROJ]; // Chance to die within 1 year
    double wxdef[MAXPROJ]; // Turnover rate within 1 year (deferred payment)
    double wximm[MAXPROJ]; // Turnover rate within 1 year (immediate payment)
    double retx[MAXPROJ]; // Chance to retire within 1 year (usually 100% at 65)
    double nPk[MAXPROJ]; // Chance to live from now until retirement
    double kPx[MAXPROJ]; // Chance to live from the start until now
    double vk[MAXPROJ]; // 1/(1+DR)^k with DR = discount rate
    double vn[MAXPROJ]; // 1/(1+DR)^n with DR = discount rate
    double vk113[MAXPROJ]; // 1/(1+DR)^k with DR = discount rate according to IAS19 $113
    double vn113[MAXPROJ]; // 1/(1+DR)^n with DR = discount rate according to IAS19 $113

    double DBORET[2][3][MAXPROJ]; // DBO Retirement (PUC - TUC, Method Assets, loops)
    double NCRET[2][3][MAXPROJ]; // Normal Cost Retirement (PUC - TUC, Method Assets, loops)
    double ICNCRET[2][3][MAXPROJ]; /* Interest Cost on Normal Cost Retirement 
			      (PUC - TUC, Method Assets, loops) */
    double assets[3][MAXPROJ]; // Plan Assets ($115, Mathematical Reserves, $113)
    double AFSL[MAXPROJ];

    //---DBO DTH---
    double CAPDTHRESPart[MAXPROJ]; // Death Lump Sum Reserves Part
    double CAPDTHRiskPart[MAXPROJ]; // Death Lump Sum Risk Part
    double DBODTHRESPart[MAXPROJ]; // DBO Death Reserves Part
    double DBODTHRiskPart[MAXPROJ]; // DBO Death Risk Part
    double NCDTHRESPart[MAXPROJ]; // NC Death Reserves Part
    double NCDTHRiskPart[MAXPROJ]; // NC Death Risk Part
    double ICNCDTHRESPart[MAXPROJ]; // Interest Cost on Normal Cost Death Reserves Part
    double ICNCDTHRiskPart[MAXPROJ]; // Interest Cost on Normal Cost Death Risk Part

    //---CASHFLOWS---
    double EBP[2][3][2][MAXPROJ]; /* Expected Benefits Paid (PUC - TUC, Method Assets,
			     PBO - TBO, loops) */
    double PBONCCF[2][3][MAXPROJ]; // PBO Normal Cost Cashflows
    double EBPDTH[2][MAXPROJ]; // Expected Benefits Paid Death
    double PBODTHNCCF[MAXPROJ]; // PBO Death Normal Cost Cashflows
} CurrentMember;

typedef struct {
    int keyrow; /* find the row in the excel file where 
		   the keys are to use in the hashtable */
    char keycolumn[4];
    char **keys; // This points to the array of keys in excel
    xmlNodePtr keynode;
    char datasheet[256]; // This is the sheet where the data lies
    unsigned int sheet; // This is the index of the sheet where the data lies
    XLfile *xl;
    Hashtable **Data; // This will be set using createData function below
    int membercnt;
    CurrentMember *cm; // This is a pointer to the affiliates	 
    UserInput *UI; /* this will point to the static UserInput struct created in 
		      userinterface.c */
    Validator *val; /* this will point to the static Validator struct created in 
		      userinterface.c */
} DataSet;

//---Useful functions for CurrentMembers---
double gensum(GenMatrix amount[], unsigned short EREE, int loop);

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
enum sensruns {runsensDuration = 21, runsensDRminus, runsensDRplus, runsensInflationminus,
    runsensInflationplus, runsensSSminus, runsensSSplus,
    runsensAgeCorrminus, runsensAgeCorrplus};
static const char *runnames[] = {"Reported Last Year", "Update Inflation",
    "Update Discount Rate", "Roll Forward", "New Data", "New Methodology", "New Mortality",
    "New Turnover", "New NRA", "New Inflation", "New Salary Increase", "New Discount Rate", 
    "New Roll Forward", "FREE", "FREE", "FREE", "FREE", "FREE", "FREE", "FREE", "FREE", 
    "Sensitivity Duration", "Sensitivity Discount Rate -", "Sensitivity Discount Rate +", 
    "Sensitivity Inflation -", "Sensitivity Inflation +", "Sensitivity Salary Increase -", 
    "Sensitivity Salary Increase +", "Sensitivity Mortality -", "Sensitivity Mortality +"}; 
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
    unsigned int lt; /* there is an array of strings containing the names of lifetables. 
			I reference the array elements by using the index defined with an 
			enum lifetables. */
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

//---Data Functions---
DataSet *createDS(Validator *, UserInput *);
CurrentMember createCM(Hashtable *);
void freeDS(DataSet *ds);
void freeCM(CurrentMember *cm);
void setGenMatrix(CurrentMember *cm, GenMatrix var[], DataColumn);
char *getcmval(CurrentMember *cm, DataColumn, int EREE, int gen);
/* This function will allocate memory based on membercnt for the underlying
   Hashtable used for the data.*/
void createData(DataSet *ds);
void setkey(DataSet *ds);
void countMembers(DataSet *ds);

//---Results Functions---
int printresults(DataSet *ds);
int printtc(DataSet *ds, unsigned int tc); // tc = test case

#endif
