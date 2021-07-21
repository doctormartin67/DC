#define _GNU_SOURCE
#include "DCProgram.h"
#include "libraryheader.h"
#include "xlsxwriter.h"

const char *colnames[MAXKEYS] = {"KEY", "NO REGLEMENT", "NAAM", "CONTRACT", "STATUS", 
    "ACTIVE CONTRACT", "SEX", "MS", "DOB", "DOE", "DOL", "DOS", "DOA", "DOR", "CATEGORIE", 
    "SAL", "PG", "PT", "NRA", "# ENF", "TARIEF", "KO", "Rent INV", "Contr INV", "ART24_A_GEN1", 
    "ART24_A_GEN2", "ART24_C_GEN1", "ART24_C_GEN2", "PREMIUM", "CAP", "CAPPS", "CAPDTH", "RES",
    "RESPS", "CAPRED", "TAUX", "DELTA_CAP_A_GEN1", "DELTA_CAP_C_GEN1", 
    "X/10", "CAO", "ORU", "CHOICE DTH", "CHOICE INV SICKNESS", "CHOICE INV WORK", "Contr_D", 
    "%ofSALforKO", "INV INDEXATION", "GR/DGR", "plan", "Baremische ancienniteit",
    "increaseSalFirstYear", "CCRA"};
static int colmissing[MAXKEYS];

static Validator *val;

DataSet *createDS(Validator *v, UserInput *UI)
{
    const char *s = UI->fname;
    Hashtable **ht;
    DataSet *ds = malloc(sizeof(DataSet));
    if (ds == NULL) errExit("[%s] malloc returned NULL\n", __func__);

    /* set all missing columns back to 0 when we try to import data again */
    for (int i = 0; i < MAXKEYS; i++)
	colmissing[i] = 0;

    val = v;

    createXLzip(s); /* THIS IS NOT PORTABLE!! */

    XLfile *xl = createXL(s);

    /* initialise DataSet */
    ds->membercnt = 0;
    ds->keys = NULL;
    ds->keynode = NULL;
    ds->Data = NULL;
    ds->xl = xl;
    ds->UI = UI;
    ds->cm = NULL;

    if (ds->xl == NULL)
    {
	updateValidation(val, ERROR, 
		"Unable to parse excel file [%s], is the file empty?", s);
	return ds;
    }
    if (!setkey(ds))
	return ds;

    countMembers(ds);
    createData(ds);

    ds->cm = (CurrentMember *)calloc(ds->membercnt, sizeof(CurrentMember));
    if (ds->cm == NULL) errExit("[%s] calloc returned NULL\n", __func__);
    ht = ds->Data;
    printf("Setting all the values for the affiliates...\n");
    for (int i = 0; i < ds->membercnt; i++)
    {
	ds->cm[i].id = i + 1;
	createCM(&ds->cm[i], *ht++);
    }
    printf("Setting values completed.\n");

    validateColumns();

    return ds;
}

// initialise all variables from data (hashtable)
void createCM(CurrentMember *cm, Hashtable *ht)
{
    cm->Data = ht;
    cm->key = getcmval(cm, KEY, -1, -1);
    cm->regl = getcmval(cm, NOREGLEMENT, -1, -1);
    cm->name = getcmval(cm, NAAM, -1, -1);
    cm->contract = getcmval(cm, CONTRACT, -1, -1);
    cm->status = 0;
    if (strcmp(getcmval(cm, STATUS, -1, -1), "ACT") == 0)  cm->status += ACT;
    if (strcmp(getcmval(cm, ACTIVECONTRACT, -1, -1), "1") == 0)  cm->status += ACTCON;
    if (strcmp(getcmval(cm, SEX, -1, -1), "1") == 0)  cm->status += MALE;
    if (strcmp(getcmval(cm, MS, -1, -1), "M") == 0)  cm->status += MARRIED;
    cm->DOB = newDate((unsigned int)atoi(getcmval(cm, DOB, -1, -1)), 0, 0, 0);
    cm->DOE = newDate((unsigned int)atoi(getcmval(cm, DOE, -1, -1)), 0, 0, 0);
    cm->DOL = newDate((unsigned int)atoi(getcmval(cm, DOL, -1, -1)), 0, 0, 0);
    cm->DOS = newDate((unsigned int)atoi(getcmval(cm, DOS, -1, -1)), 0, 0, 0);
    cm->DOA = newDate((unsigned int)atoi(getcmval(cm, DOA, -1, -1)), 0, 0, 0);
    cm->DOR = newDate((unsigned int)atoi(getcmval(cm, DOR, -1, -1)), 0, 0, 0);
    cm->category = getcmval(cm, CATEGORIE, -1, -1);
    cm->sal[0] = atof(getcmval(cm, SAL, -1, -1));
    cm->PG = atof(getcmval(cm, PG, -1, -1));
    cm->PT = atof(getcmval(cm, PT, -1, -1));
    cm->NRA = atof(getcmval(cm, NORMRA, -1, -1));
    cm->kids = (unsigned short)atoi(getcmval(cm, ENF, -1, -1));
    cm->tariff = 0;
    if (strcmp(getcmval(cm, TARIEF, -1, -1), "UKMS") == 0)  cm->tariff = UKMS;
    if (strcmp(getcmval(cm, TARIEF, -1, -1), "UKZT") == 0)  cm->tariff = UKZT;
    if (strcmp(getcmval(cm, TARIEF, -1, -1), "UKMT") == 0)  cm->tariff = UKMT;
    if (strcmp(getcmval(cm, TARIEF, -1, -1), "MIXED") == 0)  cm->tariff = MIXED;
    cm->KO = atof(getcmval(cm, KO, -1, -1));
    cm->annINV = atof(getcmval(cm, RENTINV, -1, -1));
    cm->contrINV = atof(getcmval(cm, CONTRINV, -1, -1));

    // define article 24 from data
    cm->ART24[PUC][ER][ART24GEN1][0] = atof(getcmval(cm, ART24_A_GEN1, -1, -1));
    cm->ART24[PUC][ER][ART24GEN2][0] = atof(getcmval(cm, ART24_A_GEN2, -1, -1));
    cm->ART24[PUC][EE][ART24GEN1][0] = atof(getcmval(cm, ART24_C_GEN1, -1, -1));
    cm->ART24[PUC][EE][ART24GEN2][0] = atof(getcmval(cm, ART24_C_GEN2, -1, -1));
    /* PUC = TUC = TUCPS_1 */
    for (int j = 1; j < 3; j++)
    {
	cm->ART24[j][ER][ART24GEN1][0] = cm->ART24[PUC][ER][ART24GEN1][0];
	cm->ART24[j][ER][ART24GEN2][0] = cm->ART24[PUC][ER][ART24GEN2][0];
	cm->ART24[j][EE][ART24GEN1][0] = cm->ART24[PUC][EE][ART24GEN1][0];
	cm->ART24[j][EE][ART24GEN2][0] = cm->ART24[PUC][EE][ART24GEN2][0];
    }

    // all variables that have generations, employer and employee
    //-  VARIABLES WITH MAXGEN  -
    setGenMatrix(cm, cm->PREMIUM, PREMIUM);
    setGenMatrix(cm, cm->CAP, CAP);
    setGenMatrix(cm, cm->CAPPS, CAPPS);
    setGenMatrix(cm, cm->CAPDTH, CAPDTH);
    for (int k = 0; k < TUCPS_1 + 1; k++)
    {
	setGenMatrix(cm, cm->RES[k], RES);
	setGenMatrix(cm, cm->RESPS[k], RESPS);
	setGenMatrix(cm, cm->REDCAP[k], CAPRED);
    }
    for (int j = 0; j < MAXGEN; j++)
    {
	cm->TAUX[ER][j] = atof(getcmval(cm, TAUX, ER, j + 1));
	cm->TAUX[EE][j] = atof(getcmval(cm, TAUX, EE, j + 1));      
    }

    //-  MISCELLANEOUS  -
    cm->DELTACAP[ER][0] = atof(getcmval(cm, DELTA_CAP_A_GEN1, -1, -1));
    cm->DELTACAP[EE][0] = atof(getcmval(cm, DELTA_CAP_C_GEN1, -1, -1));
    cm->X10 = atof(getcmval(cm, X10, -1, -1));
    if (cm->tariff == MIXED && cm->X10 == 0)
    {
	printf("Warning: X/10 equals zero for %s but he has a MIXED contract\n", cm->key);
	printf("X/10 will be taken as 1 by default.\n");
	cm->X10 = 1;
    }
    cm->CAO = atof(getcmval(cm, CAO, -1, -1));
    cm->ORU = getcmval(cm, ORU, -1, -1);
    cm->CHOICEDTH = getcmval(cm, CHOICEDTH, -1, -1);
    cm->CHOICEINVS = getcmval(cm, CHOICEINVS, -1, -1);
    cm->CHOICEINVW = getcmval(cm, CHOICEINVW, -1, -1);
    cm->contrDTH = atof(getcmval(cm, CONTRD, -1, -1));
    cm->percSALKO = atof(getcmval(cm, PERCOFSALFORKO, -1, -1));
    cm->indexINV = getcmval(cm, INVINDEXATION, -1, -1);
    cm->GRDGR = getcmval(cm, GRDGR, -1, -1);
    cm->plan = getcmval(cm, PLAN, -1, -1);
    cm->baranc = atof(getcmval(cm, BARANC, -1, -1));
    cm->extra = 0;
    if (strcmp(getcmval(cm, INCSALFIRSTYEAR, -1, -1), "1") == 0)  cm->extra += INCSAL;
    if (strcmp(getcmval(cm, PREP, -1, -1), "1") == 0)  cm->extra += CCRA;

    cm->DOC = (Date **)calloc(MAXPROJ, sizeof(Date *));
    if (cm->DOC == NULL) errExit("[%s] calloc returned NULL\n", __func__);
}

double gensum(GenMatrix amount[], unsigned short EREE, int loop)
{
    double sum = 0;

    for (int i = 0; i < MAXGEN; i++)
	sum += amount[EREE][i][loop];

    return sum;
}

int setkey(DataSet *ds)
{
    XLfile *xl = ds->xl;
    char *row; 
    char **xls = xl->sheetname;
    unsigned int i;
    xmlXPathObjectPtr nodeset;
    xmlNodeSetPtr nodes;
    xmlNodePtr node;

    setcol(ds->keycolumn, ds->UI->keycell);
    ds->keyrow = getrow(ds->UI->keycell);

    strcpy(ds->datasheet, ds->UI->sheetname);
    for (i = 0; i < xl->sheetcnt; i++)
	if (!strcmp(xls[i], ds->datasheet))
	    break;
    if ((ds->sheet = i) == xl->sheetcnt)
    {
	updateValidation(val, ERROR, "Sheet [%s] does not exist", ds->datasheet);
	return 0;
    }
    else
    {
	/* Check whether cell that was provided has anything in it */
	if (cell(ds->xl, ds->sheet, ds->UI->keycell) == NULL)
	{
	    updateValidation(val, ERROR, "Nothing in cell [%s] found", ds->UI->keycell);
	    return 0;
	}

	nodeset = xl->nodesets[ds->sheet];
	nodes = nodeset->nodesetval;

	/* set key node */
	for (node = *nodes->nodeTab; node != NULL; node = node->next)
	{
	    /* find node with row */
	    row = (char *)xmlGetProp(node, (const xmlChar *)"r");
	    if (atoi(row) == ds->keyrow)
	    {
		xmlFree(row);
		break;
	    }
	}

	if ((ds->keynode = node) == NULL)
	{
	    updateValidation(val, ERROR, "Nothing in cell [%s] found", ds->UI->keycell);
	    return 0;
	}
	else
	    return 1;
    }
}

void countMembers(DataSet *ds)
{
    char *row;
    int r = ds->keyrow;
    int count = ds->keyrow;
    xmlNodePtr node = ds->keynode;

    while (node != NULL)
    {
	row = (char *)xmlGetProp(node, (xmlChar *)"r");
	r = atoi(row);
	if (r - count == 1) count++; /* cells below the data should not be considered */
	node = node->next;
    }
    ds->membercnt = count - ds->keyrow;
    printf("Amount of affiliates in data: %d\n", ds->membercnt);
}

/* Excel is just a bunch of cells of the form O11, DC103, ...
   This function will create a pointer to an array of arrays of pointers to 
   hashtables, where we will store all the cells for our data. We will 
   reference cell O11 as an example going further.*/

void createData(DataSet *ds)
{
    char column[4]; // This holds the column of the beginning of our data cell, for example O
    int irow; // This holds the row of the beginning of our data cell, for example 11
    char srow[16]; /* This holds the row in string form of the beginning of our data cell, 
		      for example "11" */
    char keyCell[10]; // This will hold the cell of a key for the hashtable, for example O11
    char dataCell[10]; /* This will hold the cell of data corresponding to a key
			  for the hashtable, for example O12.*/
    char *data; // This will hold the value of the data, for example 2.391,30.

    // Here we set the initial key cell, for example B11
    strcpy(column, ds->keycolumn);
    irow = ds->keyrow;
    snprintf(srow, sizeof(srow), "%d", irow);
    strcpy(keyCell, column);
    strcat(keyCell, srow);

    // Here we set the initial data cell, for example B12
    irow++;
    snprintf(srow, sizeof(srow), "%d", irow);
    strcpy(dataCell, column);
    strcat(dataCell, srow);

    // Allocate memory for data matrix and initialise to NULL pointer
    ds->Data = (Hashtable **)calloc(ds->membercnt, sizeof(Hashtable *));
    if (ds->Data == NULL) errExit("[%s] calloc returned NULL\n", __func__);

    for (int k = 0; k < ds->membercnt; k++)
	// https://cseweb.ucsd.edu/~kube/cls/100/Lectures/lec16/lec16-8.html
	ds->Data[k] = newHashtable(233, 0); // 179 * 1.3 = 232.7 -> 233 is a prime

    // Set the keys
    int countkeys = 0;
    ds->keys = (char **)calloc(MAXKEYS, sizeof(char *));
    if (ds->keys == NULL) errExit("[%s] calloc returned NULL\n", __func__);
    char **pkey = ds->keys;
    *pkey = cell(ds->xl, ds->sheet, keyCell);

    while (*pkey != NULL)
     {
	// Here we update cell for loop, for example O11 becomes P11
	if (++countkeys >= MAXKEYS)
	    errExit("[%s] Data has too many keys\n", __func__);
	nextcol(keyCell);
	*++pkey = cell(ds->xl, ds->sheet, keyCell);
    }

    // Check for double keys
    int cntdouble = 0;
    pkey = ds->keys;
    char **pkey2 = pkey + 1;
    while (*pkey != NULL)
    {
	while (*pkey2 != NULL)
	{
	    if (strcmp(*pkey, *pkey2) == 0)
	    {
		char temp[BUFSIZ/256];
		printf("Warning: %s is a double\n", *pkey2);
		snprintf(temp, sizeof(temp), "%s%d", *pkey2, ++cntdouble + 1);
		strcpy(*pkey2, temp);
		printf("Changed it to %s\n",  *pkey2);
	    }
	    pkey2++;
	}
	pkey2 = ++pkey + 1;
    }

    // start populating Hashtable
    printf("Creating Data...\n");
    for (int i = 0; i < ds->membercnt; i++)
    {
	pkey = ds->keys;
	// Set the initial data (KEY)
	if ((data = cell(ds->xl, ds->sheet, dataCell)) == NULL)
	    lookup(*pkey++, "0", *(ds->Data + i));
	else
	    lookup(*pkey++, data, *(ds->Data + i));
	nextcol(dataCell);
	while (*pkey != NULL)
	{
	    while ((data = cell(ds->xl, ds->sheet, dataCell)) == NULL)
	    {
		if (*pkey != NULL)
		    lookup(*pkey++, "0", *(ds->Data + i));
		else
		    break;
		// Here we update cell for loop, for example O11 becomes P11
		nextcol(dataCell);
	    }

	    if (*pkey != NULL)
		lookup(*pkey++, data, *(ds->Data + i));
	    else
		break;
	    // Here we update cell for loop, for example O11 becomes P11
	    nextcol(dataCell);
	}

	// Iterate through all affiliates by updating dataCell
	irow++;
	snprintf(srow, sizeof(srow), "%d", irow);
	strcpy(dataCell, column);
	strcat(dataCell, srow);
    }
    printf("Creation complete\n");
}

int printresults(DataSet *ds)
{
    char results[PATH_MAX];
    int row = 0;
    int col = 0;  
    int index = 0;

    strcpy(results, ds->xl->dirname);
    strcat(results, "/results.xlsx");
    lxw_workbook  *workbook  = workbook_new(results);
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, "dataTY");
    worksheet_set_column(worksheet, 0, 250, 20, NULL);
    printf("Printing Data...\n");
    while (*(ds->keys + col) != NULL)
    {
	worksheet_write_string(worksheet, row, col, *(ds->keys + col), NULL);
	while (row < ds->membercnt)
	{
	    worksheet_write_string(worksheet, row+1, col,
		    lookup(*(ds->keys + col), NULL, *(ds->Data + row))->value, NULL);
	    row++;
	}
	col++;
	row = 0;
    }
    printf("Printing Data complete.\n");
    printf("Printing results...\n");

    col++;
    worksheet_write_string(worksheet, row, col+index++, "DR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "DC NC", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Method Standard", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Method DBO", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Method Assets", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Method Death", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Admin Cost", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Age", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Salary Scale", NULL);

    index++;
    worksheet_write_string(worksheet, row, col+index++, "LIAB_RET_PUC_PAR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "LIAB_RET_PUC_RES", NULL);
    worksheet_write_string(worksheet, row, col+index++, "LIAB_RET_TUC_PAR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "LIAB_RET_TUC_RES", NULL);
    worksheet_write_string(worksheet, row, col+index++, "NC_RET_PUC_PAR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "NC_RET_PUC_RES", NULL);
    worksheet_write_string(worksheet, row, col+index++, "NC_RET_TUC_PAR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "NC_RET_TUC_RES", NULL);
    worksheet_write_string(worksheet, row, col+index++, "ExpERContr", NULL);
    worksheet_write_string(worksheet, row, col+index++, "ExpEEContr", NULL);
    worksheet_write_string(worksheet, row, col+index++, "LIAB_DTH_RESPART", NULL);
    worksheet_write_string(worksheet, row, col+index++, "LIAB_DTH_RISKPART", NULL);
    worksheet_write_string(worksheet, row, col+index++, "NC_DTH_RESPART", NULL);
    worksheet_write_string(worksheet, row, col+index++, "NC_DTH_RISKPART", NULL);
    index++;
    worksheet_write_string(worksheet, row, col+index++, "Assets_PAR115", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Assets_PAR113", NULL);
    worksheet_write_string(worksheet, row, col+index++, "Assets_RES", NULL);
    worksheet_write_string(worksheet, row, col+index++, "DBO_PUC_PAR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "SC_ER_PUC_PAR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "DBO_TUC_PAR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "SC_ER_TUC_PAR", NULL);
    worksheet_write_string(worksheet, row, col+index++, "DBO_PUC_RES", NULL);
    worksheet_write_string(worksheet, row, col+index++, "SC_ER_PUC_RES", NULL);
    worksheet_write_string(worksheet, row, col+index++, "DBO_TUC_RES", NULL);
    worksheet_write_string(worksheet, row, col+index++, "SC_ER_TUC_RES", NULL);

    CurrentMember *cm = ds->cm; // address of test case member
    while (row < ds->membercnt)
    {
	index = 0;
	worksheet_write_number(worksheet, row+1, col+index++, ass.DR, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, ass.DR, NULL);
	worksheet_write_string(worksheet, row+1, col+index++, 
		(ass.method & mIAS ? "IAS" : "FAS"), NULL);
	worksheet_write_string(worksheet, row+1, col+index++, 
		(ass.method & mTUC ? "TUC" : "PUC"), NULL);
	worksheet_write_string(worksheet, row+1, col+index++, 
		(ass.method & mRES ? "RES" : 
		 (ass.method & mPAR115 ? "PAR115" : "PAR113")), NULL);
	worksheet_write_number(worksheet, row+1, col+index++, 
		(ass.method & mDTH ? 1 : 0), NULL);
	worksheet_write_number(worksheet, row+1, col+index++, tff.admincost, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, *cm->age, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, salaryscale(cm, 1), NULL);
	index++;

	double DBORETPUCPAR = sum(cm->DBORET[PUC][PAR115], MAXPROJ);
	double DBORETPUCRES = sum(cm->DBORET[PUC][MATHRES], MAXPROJ);
	double DBORETTUCPAR = sum(cm->DBORET[TUC][PAR115], MAXPROJ);
	double DBORETTUCRES = sum(cm->DBORET[TUC][MATHRES], MAXPROJ);

	double NCRETPUCPAR = sum(cm->NCRET[PUC][PAR115], MAXPROJ);
	double NCRETPUCRES = sum(cm->NCRET[PUC][MATHRES], MAXPROJ);
	double NCRETTUCPAR = sum(cm->NCRET[TUC][PAR115], MAXPROJ);
	double NCRETTUCRES = sum(cm->NCRET[TUC][MATHRES], MAXPROJ);

	double ICNCRETPUCPAR = sum(cm->ICNCRET[PUC][PAR115], MAXPROJ);
	double ICNCRETPUCRES = sum(cm->ICNCRET[PUC][MATHRES], MAXPROJ);
	double ICNCRETTUCPAR = sum(cm->ICNCRET[TUC][PAR115], MAXPROJ);
	double ICNCRETTUCRES = sum(cm->ICNCRET[TUC][MATHRES], MAXPROJ);

	double ExpERContr = 
	    max(2, 0.0, min(2, 1.0, NRA(cm, 1) - cm->age[1])) * gensum(cm->PREMIUM, ER, 1);
	double ExpEEContr = 
	    max(2, 0.0, min(2, 1.0, NRA(cm, 1) - cm->age[1])) * gensum(cm->PREMIUM, EE, 1);

	double DBODTHRESPART = sum(cm->DBODTHRESPart, MAXPROJ);
	double DBODTHRiskPART = sum(cm->DBODTHRiskPart, MAXPROJ);
	double NCDTHRESPART = sum(cm->NCDTHRESPart, MAXPROJ);
	double NCDTHRiskPART = sum(cm->NCDTHRiskPart, MAXPROJ);

	double assetsPAR115 = sum(cm->assets[PAR115], MAXPROJ);
	double assetsPAR113 = sum(cm->assets[PAR113], MAXPROJ);
	double assetsRES = sum(cm->assets[MATHRES], MAXPROJ);

	double ART24TOT = cm->ART24[PUC][ER][ART24GEN1][1] + cm->ART24[PUC][ER][ART24GEN2][1] +
	    cm->ART24[PUC][EE][ART24GEN1][1] + cm->ART24[PUC][EE][ART24GEN2][1];

	double ICNCDTHRESPART = sum(cm->ICNCDTHRESPart, MAXPROJ); 
	double ICNCDTHRiskPART = sum(cm->ICNCDTHRiskPart, MAXPROJ); 

	/* Liability */
	worksheet_write_number(worksheet, row+1, col+index++, DBORETPUCPAR, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, DBORETPUCRES, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, DBORETTUCPAR, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, DBORETTUCRES, NULL);

	/* Normal Cost */
	worksheet_write_number(worksheet, row+1, col+index++, NCRETPUCPAR, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, NCRETPUCRES, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, NCRETTUCPAR, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, NCRETTUCRES, NULL);

	/* Expected contributions */
	worksheet_write_number(worksheet, row+1, col+index++, ExpERContr, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, ExpEEContr, NULL);

	/* Death */
	worksheet_write_number(worksheet, row+1, col+index++, DBODTHRESPART, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, DBODTHRiskPART, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, NCDTHRESPART, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, NCDTHRiskPART, NULL);

	index++;
	/* Assets */
	worksheet_write_number(worksheet, row+1, col+index++, assetsPAR115, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, assetsPAR113, NULL);
	worksheet_write_number(worksheet, row+1, col+index++, assetsRES, NULL);

	/* DBO + SC */
	/* DBO RET PUC PAR */
	worksheet_write_number(worksheet, row+1, col+index++, 
		max(2, 
		    DBORETPUCPAR, 
		    (ass.method & PAR113 ? assetsPAR113 : assetsPAR115)), 
		NULL);
	/* SC ER PUC PAR */
	worksheet_write_number(worksheet, row+1, col+index++, 
		max(2, 
		    0.0, 
		    NCRETPUCPAR + ICNCRETPUCPAR - ExpEEContr), 
		NULL);
	/* DBO RET TUC PAR */
	worksheet_write_number(worksheet, row+1, col+index++, 
		max(2, 
		    DBORETTUCPAR, 
		    (ass.method & PAR113 ? assetsPAR113 : assetsPAR115)), 
		NULL);
	/* SC ER TUC PAR */
	worksheet_write_number(worksheet, row+1, col+index++, 
		max(2, 
		    (ass.method & mmaxERContr ? ExpERContr * (1 - tff.admincost) / 
		     (1 + ass.taxes) : 0.0), 
		    NCRETTUCPAR + ICNCRETTUCPAR - ExpEEContr), 
		NULL);
	/* DBO RET PUC RES */
	worksheet_write_number(worksheet, row+1, col+index++, 
		max(3, 
		    DBORETPUCRES + (ass.method & mDTH ? DBODTHRESPART : 0.0), 
		    assetsRES, 
		    ART24TOT) + (ass.method & mDTH ? DBODTHRiskPART : 0.0), 
		NULL);
	/* SC ER PUC RES */
	worksheet_write_number(worksheet, row+1, col+index++, 
		max(2, 
		    0.0,
		    NCRETPUCRES + ICNCRETPUCRES + 
		    (ass.method & mDTH ? NCDTHRESPART + ICNCDTHRESPART : 0.0) - ExpEEContr + 
		    (ass.method & mDTH ? NCDTHRiskPART + ICNCDTHRiskPART : 0.0)), 
		NULL);
	/* DBO RET TUC RES */
	worksheet_write_number(worksheet, row+1, col+index++, 
		max(3, 
		    DBORETTUCRES + (ass.method & mDTH ? DBODTHRESPART : 0.0), 
		    assetsRES, 
		    ART24TOT) + (ass.method & mDTH ? DBODTHRiskPART : 0.0), 
		NULL);
	/* SC ER TUC RES */
	worksheet_write_number(worksheet, row+1, col+index++, 
		max(2, 
		    (ass.method & mmaxERContr ? ExpERContr * (1 - tff.admincost) / 
		     (1 + ass.taxes) : 0.0),
		    NCRETTUCRES + ICNCRETTUCRES + 
		    (ass.method & mDTH ? NCDTHRESPART + ICNCDTHRESPART : 0.0) - ExpEEContr) +
		    (ass.method & mDTH ? NCDTHRiskPART + ICNCDTHRiskPART : 0.0),
		NULL);
	row++;
	cm++;
    }
    printf("Printing results complete.\n");
    return workbook_close(workbook);
}

int printtc(DataSet *ds, unsigned int tc)
{
    char results[PATH_MAX];
    char temp[64]; // to store temporary strings for field names and such.
    int row = 0;
    int col = 0;  
    int index = 0;  

    snprintf(temp, sizeof(temp), "TestCase%d", tc + 1);
    strcpy(results, ds->xl->dirname);
    strcat(results, "/");
    strcat(results, temp);
    strcat(results, ".xlsx");
    lxw_workbook *workbook = workbook_new(results);

    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, temp);
    printf("Printing Testcase...\n");
    //-  Titles of variables  -
    worksheet_write_string(worksheet, row, col++, "KEY", NULL);
    worksheet_write_string(worksheet, row, col++, "DOC", NULL);
    worksheet_write_string(worksheet, row, col++, "Age", NULL);
    col++; /* leave a column open */
    worksheet_write_string(worksheet, row, col++, "Salary", NULL);
    col += 3;
    worksheet_write_string(worksheet, row, col++, "Contr A", NULL);
    col += 3;
    worksheet_write_string(worksheet, row, col++, "DTH Risk Part", NULL);
    worksheet_write_string(worksheet, row, col++, "DTH RES Part", NULL);
    worksheet_write_string(worksheet, row, col++, "Contr C", NULL);

    for (int i = 0; i < MAXGEN; i++)
    {
	for (int j = 0; j < 2; j++) /* Employer and Employee */
	{
	    snprintf(temp, sizeof(temp), "CAP GEN %d %c", i + 1, (j == ER ? 'A' : 'C'));
	    worksheet_write_string(worksheet, row, col + 4*i + 32*j, temp, NULL);
	    snprintf(temp, sizeof(temp), "PREMIUM GEN %d %c", i + 1, (j == ER ? 'A' : 'C'));
	    worksheet_write_string(worksheet, row, col+1 + 4*i + 32*j, temp, NULL);
	    snprintf(temp, sizeof(temp), "RESERVES PS GEN %d %c", i + 1, (j == ER ? 'A' : 'C'));
	    worksheet_write_string(worksheet, row, col+2 + 4*i + 32*j, temp, NULL);
	    snprintf(temp, sizeof(temp), "RESERVES GEN %d %c", i + 1, (j == ER ? 'A' : 'C'));
	    worksheet_write_string(worksheet, row, col+3 + 4*i + 32*j, temp, NULL);
	}
    }
    col += 3 + 4*(MAXGEN-1) + 32 + 2; /* + 2 at the end is just to leave a column open */

    // Total Reserves
    worksheet_write_string(worksheet, row, col++, "Total Reserves A", NULL);
    worksheet_write_string(worksheet, row, col++, "Total Reserves C", NULL);  

    // REDCAP
    worksheet_write_string(worksheet, row, col++, "RED CAP - PUC", NULL);
    worksheet_write_string(worksheet, row, col++, "RED CAP - TUC", NULL);
    worksheet_write_string(worksheet, row, col++, "RED CAP - TUC PS+1", NULL);

    // RESERVES
    worksheet_write_string(worksheet, row, col++, "RES - PUC", NULL);
    worksheet_write_string(worksheet, row, col++, "RES - TUC", NULL);
    worksheet_write_string(worksheet, row, col++, "RES - TUC PS+1", NULL);
    col++;

    // Article 24
    for (int j = 0; j < TUCPS_1 + 1; j++)
    {
	for (int i = 0; i < 2; i++) // generation
	{
	    for (int k = 0; k < 2; k++) /* Employer and Employee */
	    {
		snprintf(temp, sizeof(temp), "ART24 GEN %d %c %s",
			i + 1, (k == ER ? 'A' : 'C'),
			(j == PUC ? "PUC" : (j == TUC ? "TUC" : "TUC PS+1")));
		worksheet_write_string(worksheet, row, col + 2*j + i + 6*k, temp, NULL);
	    }
	}
    }
    col += 2*2 + 1 + 6 + 10; /* + 10 is to move up ten columns */

    // DBO calculation
    worksheet_write_string(worksheet, row, col++, "FF", NULL);
    worksheet_write_string(worksheet, row, col++, "qx", NULL);
    worksheet_write_string(worksheet, row, col++, "wx (Deferred)", NULL);
    worksheet_write_string(worksheet, row, col++, "wx (Immediate)", NULL);
    worksheet_write_string(worksheet, row, col++, "retx", NULL);
    worksheet_write_string(worksheet, row, col++, "kPx", NULL);
    worksheet_write_string(worksheet, row, col++, "nPk", NULL);
    worksheet_write_string(worksheet, row, col++, "v^k", NULL);
    worksheet_write_string(worksheet, row, col++, "v^n", NULL);

    for (int i = 0; i < 2; i++)
	for (int j = 0; j < 3; j++)
	{
	    snprintf(temp, sizeof(temp), "DBO RET %s %s",
		    (i == PUC ? "PUC" : "TUC"),
		    (j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
	    worksheet_write_string(worksheet, row, col + j + 3*i, temp, NULL);
	    snprintf(temp, sizeof(temp), "NC RET %s %s",
		    (i == PUC ? "PUC" : "TUC"),
		    (j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
	    worksheet_write_string(worksheet, row, col + 2*3 + j + 3*i, temp, NULL);
	}
    col += 2*3 + 2 + 3 + 1;

    // Assets
    worksheet_write_string(worksheet, row, col++, "ASSETS PAR 115", NULL);
    worksheet_write_string(worksheet, row, col++, "ASSETS PAR 113", NULL);
    worksheet_write_string(worksheet, row, col++, "DBO DTH Risk Part", NULL);
    worksheet_write_string(worksheet, row, col++, "DBO DTH RES Part", NULL);
    worksheet_write_string(worksheet, row, col++, "NC DTH Risk Part", NULL);
    worksheet_write_string(worksheet, row, col++, "NC DTH RES Part", NULL);

    // EBP
    for (int j = 0; j < 3; j++) 
	for (int i = 0; i < 2; i++)
	{
	    snprintf(temp, sizeof(temp), "PBO NC CF %s %s",
		    (i == PUC ? "PUC" : "TUC"),
		    (j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
	    worksheet_write_string(worksheet, row, col + j + 3*i, temp, NULL);
	    for (int k = 0; k < 2; k++)
	    {
		snprintf(temp, sizeof(temp), "EBP %s %s %s",
			(k == TBO ? "TBO" : "PBO"),
			(i == PUC ? "PUC" : "TUC"),
			(j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
		worksheet_write_string(worksheet, row, col + 2*3 + j + 3*i + 6*k, temp, NULL);
	    }	       
	}
    col += 2*3 + 2 + 3 + 6 + 1;
    worksheet_write_string(worksheet, row, col++, "EBP DTH TBO", NULL);
    worksheet_write_string(worksheet, row, col++, "EBP DTH PBO", NULL);
    worksheet_write_string(worksheet, row, col++, "PBO DTH NC CF", NULL);

    col = 0;

    //-  Variables  -
    lxw_datetime DOC;
    lxw_format *format = workbook_add_format(workbook);
    char DOCformat[] = "dd/mm/yyyy";
    format_set_num_format(format, DOCformat);
    worksheet_set_column(worksheet, 0, 200, 20, NULL);
    CurrentMember *cm = &ds->cm[tc]; // address of test case member

    while (row < MAXPROJ)
    {
	index = 0;
	DOC.year = cm->DOC[row]->year;
	DOC.month = cm->DOC[row]->month;
	DOC.day = cm->DOC[row]->day;
	DOC.hour = DOC.min = DOC.sec = 0;
	worksheet_write_string(worksheet, row+1, col + index++, cm->key, NULL);
	worksheet_write_datetime(worksheet, row+1, col + index++, &DOC, format);
	worksheet_write_number(worksheet, row+1, col + index++, cm->age[row], NULL);
	index++;
	worksheet_write_number(worksheet, row+1, col + index++, cm->sal[row], NULL);
	index += 3;
	worksheet_write_number(worksheet, row+1, col + index++, gensum(cm->PREMIUM, ER, row), 
		NULL);
	index += 3;
	worksheet_write_number(worksheet, row+1, col + index++, cm->CAPDTHRiskPart[row], NULL);
	worksheet_write_number(worksheet, row+1, col + index++, cm->CAPDTHRESPart[row], NULL);
	worksheet_write_number(worksheet, row+1, col + index++, gensum(cm->PREMIUM, EE, row), 
		NULL);
	for (int i = 0; i < MAXGEN; i++)
	{
	    for (int j = 0; j < 2; j++)
	    {
		worksheet_write_number(worksheet, row+1, col + index + 4*i + 32*j,
			cm->CAP[j][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col + index + 1 + 4*i + 32*j,
			cm->PREMIUM[j][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col + index + 2 + 4*i + 32*j,
			cm->RESPS[PUC][j][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col + index + 3 + 4*i + 32*j,
			cm->RES[PUC][j][i][row], NULL);
	    }
	}

	index += 3 + 4*7 + 32;
	index += 2;
	// Total Reserves
	worksheet_write_number(worksheet, row+1, col + index++,
		gensum(cm->RES[PUC], ER, row) +
		gensum(cm->RESPS[PUC], ER, row), NULL);
	worksheet_write_number(worksheet, row+1, col + index++,
		gensum(cm->RES[PUC], EE, row) +
		gensum(cm->RESPS[PUC], EE, row), NULL);

	// REDCAP
	worksheet_write_number(worksheet, row+1, col + index++,
		gensum(cm->REDCAP[PUC], ER, row) +
		gensum(cm->REDCAP[PUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col + index++,
		gensum(cm->REDCAP[TUC], ER, row) +
		gensum(cm->REDCAP[TUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col + index++,
		gensum(cm->REDCAP[TUCPS_1], ER, row) +
		gensum(cm->REDCAP[TUCPS_1], EE, row), NULL);

	// RESERVES
	worksheet_write_number(worksheet, row+1, col + index++,
		gensum(cm->RES[PUC], ER, row) +
		gensum(cm->RES[PUC], EE, row) +
		gensum(cm->RESPS[PUC], ER, row) +
		gensum(cm->RESPS[PUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col + index++,
		gensum(cm->RES[TUC], ER, row) +
		gensum(cm->RES[TUC], EE, row) +
		gensum(cm->RESPS[TUC], ER, row) +
		gensum(cm->RESPS[TUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col + index++,
		gensum(cm->RES[TUCPS_1], ER, row) +
		gensum(cm->RES[TUCPS_1], EE, row) +
		gensum(cm->RESPS[TUCPS_1], ER, row) +
		gensum(cm->RESPS[TUCPS_1], EE, row), NULL);

	index++;
	// Article 24
	for (int j = 0; j < TUCPS_1 + 1; j++)
	    for (int i = 0; i < 2; i++) // generation
		for (int k = 0; k < 2; k++)
		    worksheet_write_number(worksheet, row+1, col + index + 2*j + i + 6*k,
			    cm->ART24[j][k][i][row], NULL);

	index += 2*2 + 1 + 6;
	index += 10;
	// DBO calculation
	// These variables are shifted one row down because the first row is not used
	if (row+1 < MAXPROJ)
	{
	    worksheet_write_number(worksheet, row+2, col + index++, cm->FF[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->qx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->wxdef[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->wximm[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->retx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->kPx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->nPk[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->vk[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->vn[row+1], NULL);

	    for (int i = 0; i < 2; i++)
		for (int j = 0; j < 3; j++)
		{
		    worksheet_write_number(worksheet,
			    row+2, col + index + j + 3*i, cm->DBORET[i][j][row+1], NULL);
		    worksheet_write_number(worksheet,
			    row+2, col + index + 6 + j + 3*i, cm->NCRET[i][j][row+1], NULL);
		}

	    index += 6 + 2 + 3;
	    index++;
	    // Assets
	    worksheet_write_number(worksheet, row+2, col + index++, cm->assets[PAR115][row+1], 
		    NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->assets[PAR113][row+1], 
		    NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->DBODTHRiskPart[row+1], 
		    NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->DBODTHRESPart[row+1] , 
		    NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->NCDTHRiskPart[row+1] , 
		    NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->NCDTHRESPart[row+1] , 
		    NULL);

	    // EBP
	    for (int j = 0; j < 3; j++) 
		for (int i = 0; i < 2; i++)
		{
		    worksheet_write_number(worksheet, 
			    row+2, col + index + j + 3*i, cm->PBONCCF[i][j][row+1], NULL);
		    for (int k = 0; k < 2; k++)
			worksheet_write_number(worksheet, 
				row+2, col + index + 6 + j + 3*i + 6*k, cm->EBP[i][j][k][row+1],
				NULL);
		}

	    index += 6 + 2 + 3 + 6;
	    index++;
	    worksheet_write_number(worksheet, row+2, col + index++, cm->EBPDTH[TBO][row+1], 
		    NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->EBPDTH[PBO][row+1], 
		    NULL);
	    worksheet_write_number(worksheet, row+2, col + index++, cm->PBODTHNCCF[row+1], 
		    NULL);
	}
	// END SHIFTED VARIABLES

	row++;
    }
    printf("Printing test case complete.\n");
    return workbook_close(workbook);
}

char *getcmval(CurrentMember *cm, DataColumn dc, int EREE, int gen)
{
    char value[BUFSIZ];
    strcpy(value, colnames[dc]);
    
    if (EREE >= 0 && gen > 0)
    {
	char temp[32];
	snprintf(temp, sizeof(temp), "%c%c%s%d", '_', (EREE == ER ? 'A' : 'C'), "_GEN", gen);
	strcat(value, temp);
    }

    List *h;
    if (colmissing[dc] || (h = lookup(value, NULL, cm->Data)) == NULL)
    {
	/* for now on this is set to true so this function will no longer search the hashtable
	   for the key */
	colmissing[dc] = 1;
	return strdup("0");
    }
    else
    {
	validateInput(dc, cm, value, h->value);
	return h->value;
    }
}

// Example if cm->PREMIUM then s = PREMIUM and we loop through PREMIUM_EREE_GENj
void setGenMatrix(CurrentMember *cm, GenMatrix var[], DataColumn dc)
{
    for (int j = 0; j < MAXGEN; j++)
	for (int EREE = 0; EREE < 2; EREE++)
	    var[EREE][j][0] = atof(getcmval(cm, dc, EREE, j + 1));
}

/* --- free memory functions --- */
void freeDS(DataSet *ds)
{
    if (ds == NULL)
	return;

    char **s = ds->keys;
    while (s != NULL && *s != NULL)
	free(*s++);
    free(ds->keys);

    /* to do: make freeHashtable function in hashtable.c */
    for (int i = 0; i < ds->membercnt; i++)
	freeCM(&ds->cm[i]);
    free(ds->cm);

    freeXL(ds->xl);
    
    for (int i = 0; i < ds->membercnt; i++)
	freeHashtable(ds->Data[i]);
    free(ds->Data);
    free(ds);
}

void freeCM(CurrentMember *cm)
{
    free(cm->DOB);
    free(cm->DOE);
    free(cm->DOL);
    free(cm->DOS);
    free(cm->DOA);
    free(cm->DOR);
    for (int i = 0; i < MAXPROJ; i++)
	free(cm->DOC[i]);
    free(cm->DOC);
}

void validateColumns()
{
    /* All missing columns are set to a WARNING, then the important ones are set to ERROR */
    int cnt = 0;
    for (int i = 0; i < MAXKEYS; i++)
	if (colmissing[i])
	    updateValidation(val, WARNING, 
		    "Column [%s] missing, all values set to 0", colnames[i]);

    if (colmissing[STATUS])
    {
	updateValidation(val, ERROR, "Column [%s] missing, (ACT/DEF)", colnames[STATUS]);
	cnt++;
    }

    if (colmissing[SEX])
    {
	updateValidation(val, ERROR, "Column [%s] missing, (1/2)", colnames[SEX]);
	cnt++;
    }

    if (colmissing[DOB])
    {
	updateValidation(val, ERROR, "Column [%s] missing, (Date of birth)", colnames[DOB]);
	cnt++;
    }

    if (colmissing[DOS])
    {
	updateValidation(val, ERROR, "Column [%s] missing, (Date of situation)", colnames[DOS]);
	cnt++;
    }

    if (colmissing[DOA])
    {
	updateValidation(val, ERROR, 
		"Column [%s] missing, (Date of affiliation)", colnames[DOA]);
	cnt++;
    }

    if (colmissing[DOR])
    {
	updateValidation(val, ERROR, "Column [%s] missing, (Date of retirement)", colnames[DOR]);
	cnt++;
    }

    if (colmissing[SAL])
    {
	updateValidation(val, ERROR, "Column [%s] missing, (salary)", colnames[SAL]);
	cnt++;
    }

    if (colmissing[PT])
    {
	updateValidation(val, ERROR, "Column [%s] missing, (part time)", colnames[PT]);
	cnt++;
    }

    if (colmissing[NORMRA])
    {
	updateValidation(val, ERROR, 
		"Column [%s] missing, (normal retirement age)", colnames[NORMRA]);
	cnt++;
    }

    if (colmissing[TARIEF])
    {
	updateValidation(val, ERROR, 
		"Column [%s] missing, (UKMS/UKZT/MIXED/UKMT)", colnames[TARIEF]);
	cnt++;
    }

    if (colmissing[ART24_A_GEN1])
    {
	updateValidation(val, ERROR, "Column [%s] missing", colnames[ART24_A_GEN1]);
	cnt++;
    }

    if (colmissing[ART24_A_GEN2])
    {
	updateValidation(val, ERROR, "Column [%s] missing", colnames[ART24_A_GEN2]);
	cnt++;
    }

    if (colmissing[ART24_C_GEN1])
    {
	updateValidation(val, ERROR, "Column [%s] missing", colnames[ART24_C_GEN1]);
	cnt++;
    }

    if (colmissing[ART24_C_GEN2])
    {
	updateValidation(val, ERROR, "Column [%s] missing", colnames[ART24_C_GEN2]);
	cnt++;
    }

    if (colmissing[RES])
    {
	updateValidation(val, ERROR, 
		"One or more of the generational columns for [%s] are missing", 
		colnames[RES]);
	cnt++;
    }

    /* after 10 missing columns an error message in included with a suggestion that the 
       wrong cell was chosen */
    if (cnt > 10)
	updateValidation(val, ERROR, 
		"\nMany columns missing, possible reasons:\n"
		"- incorrect cell chosen under the \"Data\" section\n"
		"- there is an empty column name in the data, "
		"the search for columns will end there");
}

void validateInput(DataColumn dc, CurrentMember *cm, const char *key, const char *input)
{
    /* not all columns were added here because not all of them will be used in the
       program, I chose the most important ones to check, the others are the responsibility of 
       the user */
    DataColumn floats[] = {SAL, PT, NORMRA, ART24_A_GEN1, ART24_A_GEN2, ART24_C_GEN1, 
	ART24_C_GEN2, PREMIUM, CAP, CAPPS, RES, RESPS, CAPRED, TAUX};
    DataColumn dates[] = {DOB, DOS, DOA, DOR};

    int lenf = sizeof(floats)/sizeof(floats[0]);
    int lend = sizeof(dates)/sizeof(dates[0]);

    /* --- Check floats --- */
    for (int i = 0; i < lenf; i++)
    {
	if (dc == floats[i])
	{
	    if (!isfloat(input))
		updateValidation(val, ERROR, 
			"Member [%d][%s] has [%s = %s], expected of the form %s", 
			cm->id, cm->key, key, input, validMsg[FLOATERR]);
	    if (input[0] == '-')
		updateValidation(val, WARNING, 
			"Member [%d][%s] has a negative %s [%s]", cm->id, cm->key, key, input);
	    return;
	}
    }

    /* --- Check Dates --- */
    for (int i = 0; i < lend; i++)
    {
	if (dc == dates[i])
	{
	    Date *temp = newDate((unsigned int)atoi(input), 0, 0, 0);
	    if (temp == NULL)
		updateValidation(val, ERROR, 
			"Member [%d][%s] has invalid date [%s = %s]", 
			cm->id, cm->key, key, input); 
	    free(temp);
	    return;
	}
    }

    /* --- Check Miscellaneous --- */
    if (dc == STATUS)
    {
	if (strcmp(input, "ACT") != 0 && strcmp(input, "DEF") != 0)
	    updateValidation(val, ERROR, 
		    "Member [%d][%s] has invalid status [%s = %s], expected ACT or DEF", 
		    cm->id, cm->key, key, input); 
	return;
    }

    if (dc == SEX)
    {
	if (atoi(input) != 1 && atoi(input) != 2)
	    updateValidation(val, ERROR, 
		    "Member [%d][%s] has invalid gender [%s = %s], "
		    "expected 1(male) or 2(female)", cm->id, cm->key, key, input); 
	return;
    }

    if (dc == PT)
    {
	if (atof(input) > 1 || atof(input) < 0)
	    updateValidation(val, WARNING, 
		    "Member [%d][%s] has [%s = %s], expected between 0 and 1", 
		    cm->id, cm->key, key, input); 
	return;
    }
}

/* --- Assumptions functions --- */
double salaryscale(CurrentMember *cm, int k)
{
    return (*ass.SS)(cm, k);
}

double calcA(CurrentMember *cm, int k)
{
    return (*ass.calcA)(cm, k);
}

double calcC(CurrentMember *cm, int k)
{
    return (*ass.calcC)(cm, k);
}

double calcDTH(CurrentMember *cm, int k)
{
    return (*ass.calcDTH)(cm, k);
}

double NRA(CurrentMember *cm, int k)
{
    return (*ass.NRA)(cm, k);
}

double wxdef(CurrentMember *cm, int k)
{
    return (*ass.wxdef)(cm, k);
}

double retx(CurrentMember *cm, int k)
{
    return (*ass.retx)(cm, k);
}
