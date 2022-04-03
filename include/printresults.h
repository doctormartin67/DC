#ifndef PRINTRESULTS
#define PRINTRESULTS

/*
 * defines some errors that can occur during printing of results
 */
enum {NO_PRERR, NAME_PRERR};

/*
 * The following enum defines the column (starting at 0) that the test results
 * will be printed at. To leave a column or more open an integer is added to
 * the appropriate column
 */
enum {
	TC_KEY, TC_DOC, TC_AGE, TC_SAL = TC_AGE + 2, TC_NDOA = TC_SAL + 2,
	TC_NDOE, TC_CONTRA, TC_DTHRISK = TC_CONTRA + 4, TC_DTHRES, TC_CONTRC,

	/* These are special because they have multiple generations */
	TC_CAP, TC_PREM, TC_RESPS, TC_RES,

	TC_TOTRESA = TC_CONTRC + EREE_AMOUNT * 4 * MAXGEN + 2,
	TC_TOTRESC, TC_REDCAPPUC, TC_REDCAPTUC, TC_REDCAPTUCPS1, TC_RESPUC,
	TC_RESTUC, TC_RESTUCPS1,

	/* This is special because it has multiple generations */
	TC_ART24 = TC_RESTUCPS1 + 2,
	TC_FF = TC_ART24 + EREE_AMOUNT * METHOD_AMOUNT * ART24GEN_AMOUNT + 9,
	TC_QX, TC_WXDEF, TC_WXIMM, TC_RETX, TC_KPX, TC_NPK, TC_VK, TC_VN,

	/* These are special because they have multiple methods */
	TC_DBO, TC_NC,

	TC_ASSETS115 = TC_VN + 1 + 2 * (METHOD_AMOUNT - 1) * ASSET_AMOUNT,
	TC_ASSETS113, TC_DBODTHRISK, TC_DBODTHRES, TC_NCDTHRISK, TC_NCDTHRES,

	/* These are special because they have multiple methods */
	TC_PBONCCF, TC_EBP,

	TC_EBPDTHTBO = TC_PBONCCF + 2 * ASSET_AMOUNT * (METHOD_AMOUNT - 1)
	+ ASSET_AMOUNT * CF_AMOUNT, TC_EBPDTHPBO, TC_PBODTHNCCF, TC_AMOUNT
};

/*
 * create an excel file called TestCase[tc] with all the values for a given
 * person in the data to test the results.
 */
void print_test_case(const CurrentMember *cm);
void print_results(const Database *db, const CurrentMember *cm);
unsigned printtc(CurrentMember *cm, unsigned tc);

#endif
