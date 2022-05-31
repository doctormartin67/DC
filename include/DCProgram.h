#ifndef DCPROGRAM
#define DCPROGRAM

#include <stdio.h>
#include <gtk/gtk.h>
#include "dates.h"
#include "validation.h"
#include "inputdata.h"
#include "excel.h"

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
enum {ER, EE, EREE_AMOUNT};// Employer index, Employer index
// article 24 index (3,75% for employee contributions and 3,25% for employee)
// article 24 index (1,75%)
enum {ART24GEN1, ART24GEN2, ART24GEN_AMOUNT};
#define ART24admincost 0.05 // Maximum admin cost that may be applied to Employer contribution
enum {UKMS, UKZT, UKMT, MIXED, INSCOMB_AMOUNT};
extern const char *const inscomb[INSCOMB_AMOUNT];
// projected unit credit with future premiums
// projected unit credit without future premiums
/* projected unit credit with future premiums,
   one year later (for service cost)*/
// don't alter this order!!
enum {PUC, TUC, TUCPS_1, METHOD_AMOUNT};
// Assets $115
// Assets Mathematical Reserves
// Assets $113
enum {PAR115, MATHRES, PAR113, ASSET_AMOUNT};
// PBO Cashflows
// TBO Cashflows
enum cashflows {PBO, TBO, CF_AMOUNT};

enum {INTERPRETERTEXT_SIZE = 4096};

static const double art24_interest_rates[EREE_AMOUNT][ART24GEN_AMOUNT] = {
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

//---Data Declarations---

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

struct methods {
	double puc; //projected unit credit
	double tuc; //traditional unit credit
	double tucps_1; //traditional unit credit +1 service year
};

struct reserves {
	double ps[EREE_AMOUNT];
	struct methods res[EREE_AMOUNT];
};

struct lump_sums {
	double lump_sum[EREE_AMOUNT];
	double ps[EREE_AMOUNT];
	double death_lump_sum[EREE_AMOUNT];
	struct methods reduced[EREE_AMOUNT];
};

typedef enum {
	GENS_NONE,
	GENS_PREMIUM_A,
	GENS_PREMIUM_C,
	GENS_RP_A,
	GENS_RP_C,
	GENS_DEATH_CAP_A,
	GENS_DEATH_CAP_C,
	GENS_LUMP_SUM,
	GENS_RES,
} GenerationKind;

struct generations {
	double premium[EREE_AMOUNT];
	double risk_premium[EREE_AMOUNT];
	struct lump_sums lump_sums;
	struct reserves reserves;
};

struct art24 {
	double i[EREE_AMOUNT];
	struct methods res[EREE_AMOUNT];
};

//---Variables that are used for DBO calculation---
struct factor {
	double ff; // Funding Factor
	double ff_sc; // Funding Factor Service Cost
	double qx; // Chance to die within 1 year
	double wxdef; // Turnover rate for 1 year (deferred payment)
	double wximm; // Turnover rate for 1 year (immediate payment)
	double retx; // Retirement rate for 1 year (mostly 100% at 65)
	double nPk; // Chance to live from now until retirement
	double kPx; // Chance to live from the start until now
	double vk; // 1/(1+DR)^k with DR = discount rate
	double vn; // 1/(1+DR)^n with DR = discount rate
	double vk113; /* 1/(1+DR)^k with DR = discount rate
				  according to IAS19 $113 */
	double vn113; /* 1/(1+DR)^n with DR = discount rate
				  according to IAS19 $113 */
};

struct assets {
	double math_res;
	double par115;
	double par113;
};

struct death {
	double death_res; // Death Reserves Part
	double death_risk; // Death Risk Part
	double ic_death_res; // interest cost
	double ic_death_risk; // interest cost
};

struct retirement {
	double puc[ASSET_AMOUNT];
	double tuc[ASSET_AMOUNT];
};

typedef enum {
	PROJ_NONE,
	PROJ_AGE,
	PROJ_NDOE,
	PROJ_NDOA,
	PROJ_SAL,
	PROJ_AFSL,
	PROJ_DEATH_RES,
	PROJ_DEATH_RISK,
	PROJ_PBO_NC_DEATH,
	PROJ_DELTA_CAP_A,
	PROJ_DELTA_CAP_C,
	PROJ_EBP_DEATH_PBO,
	PROJ_EBP_DEATH_TBO,
	PROJ_DOC,
	PROJ_GENS,
	PROJ_ART24,
	PROJ_FACTOR,
	PROJ_DBO_RET,
	PROJ_NC_RET,
	PROJ_IC_NC_RET,
	PROJ_EBP_RET_PBO,
	PROJ_EBP_RET_TBO,
	PROJ_PBO_NC_RET,
	PROJ_DBO_DEATH,
	PROJ_NC_DEATH,
	PROJ_ASSETS,
} ProjectionKind;

struct projection {
	double age;
	double nDOE; /* years since date of entry */
	double nDOA; /* years since date of affiliation */
	double sal;
	double afsl;
	double death_res; // Death Lump Sum Reserves Part
	double death_risk; // Death Lump Sum Risk Part
	double pbo_nc_death;
	double delta_cap[EREE_AMOUNT];
	double ebp_death[CF_AMOUNT];
	struct date *DOC; // date of calculation
	struct generations gens[MAXGEN];
	struct art24 art24[ART24GEN_AMOUNT];
	struct factor factor;
	struct retirement dbo_ret;
	struct retirement nc_ret;
	struct retirement ic_nc_ret;
	struct retirement ebp_ret[CF_AMOUNT];
	struct retirement pbo_nc_ret;
	struct death dbo_death;
	struct death nc_death;
	struct assets assets;
};

typedef struct {
	size_t id; /* Used to print warning/error messages to user */

	//---Variable Declarations---  
	const char *key; // KEY
	const char *regl; // REGLEMENT (plan rule)
	const char *name; // NAME
	const char *contract; // CONTRACT number
	unsigned status; /* 0000 0000 0000 0111 means single male active member
			    and active contract */
	struct date *DOB; // date of birth
	struct date *DOE; // date of entry
	struct date *DOL; // date of leaving
	struct date *DOS; // date of situation
	struct date *DOA; // date of affiliation
	struct date *DOR; // date of retirement
	const char *category; // f.e. blue collar, white collar, management, ...
	double PG; // pensioengrondslag (I have never needed this)
	double PT; // part time
	double NRA; // normal retirement age
	unsigned kids; // amount of kids
	unsigned tariff; // UKMS, UKZT, UKMT, MIXED

	double TAUX[EREE_AMOUNT][MAXGEN]; /* return guarentee insurer */
	double X10; // MIXED combination

	struct projection proj[MAXPROJ + 1];
} CurrentMember;

//---Useful functions for CurrentMembers---
double get_method_amount(struct methods m, size_t method);
double gen_sum(const struct generations *, size_t EREE, DataColumn,
		size_t method);
double gen_sum_art24(const struct art24 *a24, size_t method);
double proj_sum(ProjectionKind , const struct projection *, size_t method,
		size_t asset_kind);

//---Tariff Structure---
typedef struct {
	const char *table;
	double i; // Insurance rate (This changes for prolongation table for example)
} LifeTable;

typedef struct {
	LifeTable ltINS[EREE_AMOUNT][MAXGEN]; // Insurer
	LifeTable ltAfterTRM[EREE_AMOUNT][MAXGEN]; // after termination
	LifeTable ltProlong[EREE_AMOUNT]; // Prolongation
	LifeTable ltProlongAfterTRM[EREE_AMOUNT];
	double costRES; // Cost on reserves
	double WDDTH; // Profit sharing death (winstdeelname)
	double costKO; // cost on Death lump sum (kapitaal overlijden)
	double admincost; // Administration cost
	double MIXEDPS;
	unsigned prepost; // prenumerando-postnumerando
	unsigned term; // term of payments
} Tariff;

Tariff tff; // Tariff structure

//---Data Functions---
CurrentMember *create_members(const Database *db);
void validate_columns(void);
void validate_input(DataColumn dc, const CurrentMember *cm, const char *key,
		const char *input);

#endif
