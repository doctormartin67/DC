#ifndef DCPROGRAM
#define DCPROGRAM

#include <stdio.h>
#include <gtk/gtk.h>
#include "hashtable.h"
#include "dates.h"
#include "XL.h"
#include "validation.h"
#include "inputdata.h"

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
			   the contributions and the service cost
			   (never used in case we have PUC methodology) */
#define mmaxPUCTUC 0x10 // used to set bit on if we take maximum of PUC and TUC
#define mRES 0x20 // used to set bit in case assets are mathematical reserves
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
enum {ER, EE, EREE_AMOUNT};// Employer index, Employer index
// article 24 index (3,75% for employee contributions and 3,25% for employee)
// article 24 index (1,75%)
enum {ART24GEN1, ART24GEN2, ART24GEN_AMOUNT};
#define ART24admincost 0.05 // Maximum admin cost that may be applied to Employer contribution
enum inscomb {UKMS, UKZT, UKMT, MIXED};
// projected unit credit with future premiums
// projected unit credit without future premiums
/* projected unit credit with future premiums,
   one year later (for service cost)*/
// don't alter this order!!
enum {PUC, TUC, TUCPS_1, METHOD_AMOUNT};
// Assets $115
// Assets Mathematical Reserves
// Assets $113
enum assets {PAR115, MATHRES, PAR113, ASSET_AMOUNT};
// PBO Cashflows
// TBO Cashflows
enum cashflows {PBO, TBO, CF_AMOUNT};

static const double ART24TAUX[EREE_AMOUNT][ART24GEN_AMOUNT] = {
	[ER] = {
		[ART24GEN1] = 0.0325, 
		[ART24GEN2] = 0.0175
	}, 
	[EE] = {
		[ART24GEN1] = 0.0375, 
		[ART24GEN2] = 0.0175
	}
};
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

/*
 * the following typedef is maybe the most important part of the entire
 * program. Each member of a pension plan has characteristics that need to be
 * given in the excel file that is used to run. All of the data is
 * collected and stored in 'CurrentMember' for each member of the plan.
 * Each column in the excel file corresponds to one of the variables in
 * CurrentMember under "Variable Declarations"
 * Under "Variable Definitions" we define extra variables needed in each
 * run of a member
 */
typedef struct {
	Hashtable *Data;
	unsigned id; /* Used to print warning/error messages to user */

	//---Variable Declarations---  
	char *key; // KEY
	char *regl; // REGLEMENT (plan rule)
	char *name; // NAME
	char *contract; // CONTRACT number
	unsigned status; /* 0000 0000 0000 0111 means single male active member
			    and active contract */
	struct date *DOB; // date of birth
	struct date *DOE; // date of entry
	struct date *DOL; // date of leaving
	struct date *DOS; // date of situation
	struct date *DOA; // date of affiliation
	struct date *DOR; // date of retirement
	struct date *DOC[MAXPROJ + 1]; // date of calculation
	char *category; // f.e. blue collar, white collar, management, ...
	double sal[MAXPROJ]; // salary
	double PG; // pensioengrondslag (I have never needed this)
	double PT; // part time
	double NRA; // normal retirement age
	unsigned kids; // amount of kids
	unsigned tariff; // UKMS, UKZT, UKMT, MIXED
	double KO; // death lump sum (kapitaal overlijden)
	double annINV; // annuity in case of invalidity
	double contrINV; // contribution for invalidity insurance

	/*
	 * Article 24 of the Belgium law (WAP)
	 */
	double ART24[METHOD_AMOUNT][EREE_AMOUNT][ART24GEN_AMOUNT][MAXPROJ]; 
	GenMatrix CAP[EREE_AMOUNT]; // Pension lump sum
	GenMatrix CAPPS[EREE_AMOUNT]; /* Pension lump sum profit sharing */
	GenMatrix REDCAP[METHOD_AMOUNT][EREE_AMOUNT]; /* Reduced lump sum */
	double TAUX[EREE_AMOUNT][MAXGEN]; /* return guarentee insurer */
	GenMatrix PREMIUM[EREE_AMOUNT];
	GenMatrix RES[METHOD_AMOUNT][EREE_AMOUNT]; // Reserves
	GenMatrix RESPS[METHOD_AMOUNT][EREE_AMOUNT]; /* Profit Sharing */
	double DELTACAP[EREE_AMOUNT][MAXPROJ]; // Delta Cap (AXA)
	double X10; // MIXED combination
	GenMatrix CAPDTH[EREE_AMOUNT]; /* Death lump sum (used for UKMT) */
	GenMatrix RP[EREE_AMOUNT]; // Risk Premium
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
	unsigned extra; /* 0000 0000 0000 0011 means prepensioner whose salary
			   we increase at k = -1 */

	//---Variable Definitions---    
	double age[MAXPROJ + 1];
	double nDOE[MAXPROJ]; // years since date of entry
	double nDOA[MAXPROJ]; // years since date of affiliation

	//---Variables that are used for DBO calculation---
	double FF[MAXPROJ]; // Funding Factor
	double FFSC[MAXPROJ]; // Funding Factor Service Cost
	double qx[MAXPROJ]; // Chance to die within 1 year
	double wxdef[MAXPROJ]; // Turnover rate for 1 year (deferred payment)
	double wximm[MAXPROJ]; // Turnover rate for 1 year (immediate payment)
	double retx[MAXPROJ]; // Retirement rate for 1 year (mostly 100% at 65)
	double nPk[MAXPROJ]; // Chance to live from now until retirement
	double kPx[MAXPROJ]; // Chance to live from the start until now
	double vk[MAXPROJ]; // 1/(1+DR)^k with DR = discount rate
	double vn[MAXPROJ]; // 1/(1+DR)^n with DR = discount rate
	double vk113[MAXPROJ]; /* 1/(1+DR)^k with DR = discount rate
				  according to IAS19 $113 */
	double vn113[MAXPROJ]; /* 1/(1+DR)^n with DR = discount rate
				  according to IAS19 $113 */

	// Retirement
	double DBORET[METHOD_AMOUNT-1][ASSET_AMOUNT][MAXPROJ]; // DBO
	double NCRET[METHOD_AMOUNT-1][ASSET_AMOUNT][MAXPROJ]; // Normal Cost
	// Interest Cost on Normal Cost 
	double ICNCRET[METHOD_AMOUNT-1][ASSET_AMOUNT][MAXPROJ]; 
	double assets[ASSET_AMOUNT][MAXPROJ];
	double AFSL[MAXPROJ];

	//---DBO DTH---
	double CAPDTHRESPart[MAXPROJ]; // Death Lump Sum Reserves Part
	double CAPDTHRiskPart[MAXPROJ]; // Death Lump Sum Risk Part
	double DBODTHRESPart[MAXPROJ]; // DBO Death Reserves Part
	double DBODTHRiskPart[MAXPROJ]; // DBO Death Risk Part
	double NCDTHRESPart[MAXPROJ]; // NC Death Reserves Part
	double NCDTHRiskPart[MAXPROJ]; // NC Death Risk Part
	double ICNCDTHRESPart[MAXPROJ]; /* Interest Cost on Normal Cost Death
					   Reserves Part */
	double ICNCDTHRiskPart[MAXPROJ]; /* Interest Cost on Normal Cost Death
					    Risk Part */

	//---CASHFLOWS---
	double EBP[METHOD_AMOUNT][ASSET_AMOUNT][CF_AMOUNT][MAXPROJ];
	double PBONCCF[METHOD_AMOUNT][ASSET_AMOUNT][MAXPROJ]; // Normal Cost
	double EBPDTH[METHOD_AMOUNT][MAXPROJ]; // Expected Benefits Paid Death
	double PBODTHNCCF[MAXPROJ]; // PBO Death Normal Cost Cashflows
} CurrentMember;

/*
 * When the XLfile is created from an excel file, the properties of the
 * parts of that file are stored in a struct DataSet.
 * In particular, we need to know
 * - the excel file (xl) to retrieve data
 * - where the data starts (cell with keyrow and keycolumn)
 * - all the keys that the hashtable will use
 * - the keynode used for libxml
 * - the sheet that the data lies in
 * - the amount of members in the data
 * - the user input that the data will run with (f.e. the assumptions)
 */
typedef struct {
	unsigned keyrow;
	unsigned sheet;
	unsigned membercnt;
	char **keys;
	char keycolumn[4];
	char datasheet[256];
	xmlNodePtr keynode;
	XLfile *xl;
	Hashtable **Data;
	CurrentMember *cm;
	UserInput *UI; 
} DataSet;

//---Useful functions for CurrentMembers---
double gensum(const GenMatrix amount[], unsigned EREE, unsigned loop);

//---Assumptions declarations---
typedef struct {
	struct date *DOC; /* This is DOC[1] which is the start of the run through affiliates.
		      DOC[0] is date of situation.*/
	double infl; // Inflation
	double DR; // Discount Rate
	double DR113; // Discount Rate under $113 of IAS19  
	short agecorr; // Age Correction
	double (*SS)(const CurrentMember *cm, int k); // Salary Scale
	double (*calcA)(const CurrentMember *cm, int k); // Formula for Employer retirement contribution
	double (*calcC)(const CurrentMember *cm, int k); // Formula for Employee retirement contribution  
	double (*calcDTH)(const CurrentMember *cm, int k); // Formula for Capital Death
	double (*NRA)(const CurrentMember *cm, int k);
	double (*wxdef)(const CurrentMember *cm, int k); // Turnover rate with deferred payment
	double (*retx)(const CurrentMember *cm, int k); // Turnover rate with deferred payment

	//Assumptions that usually won't change from year to year
	unsigned incrSalk0; // determine whether sal gets increased at k = 0 
	unsigned incrSalk1; // determine whether sal gets increased at k = 1 
	double TRM_PercDef; /* Percentage of deferred members that will keep their
			       reserves with the current employer at termination (usually equals 1)*/
	unsigned method; // Methodology, we use bits here
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
unsigned currrun; // Current run
void setassumptions(CurrentMember *cm, UserInput *UILY, UserInput *UITY);
double salaryscale(const CurrentMember *cm, int k);
double calcA(const CurrentMember *cm, int k);
double calcC(const CurrentMember *cm, int k);
double calcDTH(const CurrentMember *cm, int k);
double NRA(const CurrentMember *cm, int k);
double wxdef(const CurrentMember *cm, int k);
double retx(const CurrentMember *cm, int k);

//---Tariff Structure---
typedef struct {
	unsigned lt; /* there is an array of strings containing the names of lifetables. 
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
void createCM(CurrentMember *, Hashtable *);
void freeDS(DataSet *ds);
void freeCM(CurrentMember *cm);
void setGenMatrix(CurrentMember *cm, GenMatrix var[], DataColumn);
char *getcmval(CurrentMember *cm, DataColumn, int EREE, int gen);
void validateColumns();
void validateInput(DataColumn dc, const CurrentMember *cm, const char *key,
		const char *input);
/* This function will allocate memory based on membercnt for the underlying
   Hashtable used for the data.*/
void createData(DataSet *ds);
/* returns 1 if sheet and row were found, otherwise returns 0 */
int setkey(DataSet *ds);
void countMembers(DataSet *ds);

//---Results Functions---
int printresults(DataSet *ds);
int printtc(DataSet *ds, unsigned tc); // tc = test case

#endif
