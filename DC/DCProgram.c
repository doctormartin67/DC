#define _GNU_SOURCE
#include "DCProgram.h"
#include "libraryheader.h"
#include "xlsxwriter.h"

void setDSvals(XLfile *xl, DataSet *ds)
{
    ds->xl = xl;
    setkey(ds);
    countMembers(ds);
    createData(ds);
}

// initialise all variables from data (hashtable)
void setCMvals(DataSet *ds)
{
    CurrentMember *cm;
    ds->cm = (CurrentMember *)calloc(ds->membercnt, sizeof(CurrentMember));
    if ((cm = ds->cm) == NULL) errExit("[%s] calloc returned NULL\n", __func__);

    printf("Setting all the values for the affiliates...\n");
    for (int i = 0; i < ds->membercnt; i++)
    {
	cm[i].Data = *(ds->Data + i);
	cm[i].key = getcmval(&cm[i], KEY, -1, -1);
	cm[i].regl = getcmval(&cm[i], NOREGLEMENT, -1, -1);
	cm[i].name = getcmval(&cm[i], NAAM, -1, -1);
	cm[i].contract = getcmval(&cm[i], CONTRACT, -1, -1);
	cm[i].status = 0;
	if (strcmp(getcmval(&cm[i], STATUS, -1, -1), "ACT") == 0)  cm[i].status += ACT;
	if (strcmp(getcmval(&cm[i], ACTIVECONTRACT, -1, -1), "1") == 0)  cm[i].status += ACTCON;
	if (strcmp(getcmval(&cm[i], SEX, -1, -1), "1") == 0)  cm[i].status += MALE;
	if (strcmp(getcmval(&cm[i], MS, -1, -1), "M") == 0)  cm[i].status += MARRIED;
	cm[i].DOB = newDate((unsigned int)atoi(getcmval(&cm[i], DOB, -1, -1)), 0, 0, 0);
	cm[i].DOE = newDate((unsigned int)atoi(getcmval(&cm[i], DOE, -1, -1)), 0, 0, 0);
	cm[i].DOL = newDate((unsigned int)atoi(getcmval(&cm[i], DOL, -1, -1)), 0, 0, 0);
	cm[i].DOS = newDate((unsigned int)atoi(getcmval(&cm[i], DOS, -1, -1)), 0, 0, 0);
	cm[i].DOA = newDate((unsigned int)atoi(getcmval(&cm[i], DOA, -1, -1)), 0, 0, 0);
	cm[i].DOR = newDate((unsigned int)atoi(getcmval(&cm[i], DOR, -1, -1)), 0, 0, 0);
	cm[i].category = getcmval(&cm[i], CATEGORIE, -1, -1);
	cm[i].sal[0] = atof(getcmval(&cm[i], SAL, -1, -1));
	cm[i].PG = atof(getcmval(&cm[i], PG, -1, -1));
	cm[i].PT = atof(getcmval(&cm[i], PT, -1, -1));
	cm[i].NRA = atof(getcmval(&cm[i], RA, -1, -1));
	cm[i].kids = (unsigned short)atoi(getcmval(&cm[i], ENF, -1, -1));
	cm[i].tariff = 0;
	if (strcmp(getcmval(&cm[i], TARIEF, -1, -1), "UKMS") == 0)  cm[i].tariff = UKMS;
	if (strcmp(getcmval(&cm[i], TARIEF, -1, -1), "UKZT") == 0)  cm[i].tariff = UKZT;
	if (strcmp(getcmval(&cm[i], TARIEF, -1, -1), "UKMT") == 0)  cm[i].tariff = UKMT;
	if (strcmp(getcmval(&cm[i], TARIEF, -1, -1), "MIXED") == 0)  cm[i].tariff = MIXED;
	cm[i].KO = atof(getcmval(&cm[i], KO, -1, -1));
	cm[i].annINV = atof(getcmval(&cm[i], RENTINV, -1, -1));
	cm[i].contrINV = atof(getcmval(&cm[i], CONTRINV, -1, -1));

	// define article 24 from data
	for (int j = 0; j < TUCPS_1 + 1; j++)
	{
	    cm[i].ART24[j][ER][ART24GEN1][0] = atof(getcmval(&cm[i], ART24_A_GEN1, -1, -1));
	    cm[i].ART24[j][ER][ART24GEN2][0] = atof(getcmval(&cm[i], ART24_A_GEN2, -1, -1));
	    cm[i].ART24[j][EE][ART24GEN1][0] = atof(getcmval(&cm[i], ART24_C_GEN1, -1, -1));
	    cm[i].ART24[j][EE][ART24GEN2][0] = atof(getcmval(&cm[i], ART24_C_GEN2, -1, -1));
	}

	// all variables that have generations, employer and employee
	//-  VARIABLES WITH MAXGEN  -
	setGenMatrix(&cm[i], cm[i].PREMIUM, PREMIUM);
	setGenMatrix(&cm[i], cm[i].CAP, CAP);
	setGenMatrix(&cm[i], cm[i].CAPPS, CAPPS);
	setGenMatrix(&cm[i], cm[i].CAPDTH, CAPDTH);
	for (int k = 0; k < TUCPS_1 + 1; k++)
	{
	    setGenMatrix(&cm[i], cm[i].RES[k], RES);
	    setGenMatrix(&cm[i], cm[i].RESPS[k], RESPS);
	    setGenMatrix(&cm[i], cm[i].REDCAP[k], CAPRED);
	}
	for (int j = 0; j < MAXGEN; j++)
	{
	    cm[i].TAUX[ER][j] = atof(getcmval(&cm[i], TAUX, ER, j + 1));
	    cm[i].TAUX[EE][j] = atof(getcmval(&cm[i], TAUX, EE, j + 1));      
	}

	//-  MISCELLANEOUS  -
	cm[i].DELTACAP[ER][0] = atof(getcmval(&cm[i], DELTA_CAP_A_GEN1, -1, -1));
	cm[i].DELTACAP[EE][0] = atof(getcmval(&cm[i], DELTA_CAP_C_GEN1, -1, -1));
	cm[i].X10 = atof(getcmval(&cm[i], X10, -1, -1));
	if (cm[i].tariff == MIXED && cm[i].X10 == 0)
	{
	    printf("Warning: X/10 equals zero for %s but he has a MIXED contract\n", cm[i].key);
	    printf("X/10 will be taken as 1 by default.\n");
	    cm[i].X10 = 1;
	}
	cm[i].CAO = atof(getcmval(&cm[i], CAO, -1, -1));
	cm[i].ORU = getcmval(&cm[i], ORU, -1, -1);
	cm[i].CHOICEDTH = getcmval(&cm[i], CHOICEDTH, -1, -1);
	cm[i].CHOICEINVS = getcmval(&cm[i], CHOICEINVS, -1, -1);
	cm[i].CHOICEINVW = getcmval(&cm[i], CHOICEINVW, -1, -1);
	cm[i].contrDTH = atof(getcmval(&cm[i], CONTRD, -1, -1));
	cm[i].percSALKO = atof(getcmval(&cm[i], PERCOFSALFORKO, -1, -1));
	cm[i].indexINV = getcmval(&cm[i], INVINDEXATION, -1, -1);
	cm[i].GRDGR = getcmval(&cm[i], GRDGR, -1, -1);
	cm[i].plan = getcmval(&cm[i], PLAN, -1, -1);
	cm[i].baranc = atof(getcmval(&cm[i], BARANC, -1, -1));
	cm[i].extra = 0;
	if (strcmp(getcmval(&cm[i], INCSALFIRSTYEAR, -1, -1), "1") == 0)  cm[i].extra += INCSAL;
	if (strcmp(getcmval(&cm[i], PREP, -1, -1), "1") == 0)  cm[i].extra += CCRA;

	cm[i].DOC = (Date **)calloc(MAXPROJ, sizeof(Date *));
	if (cm[i].DOC == NULL) errExit("[%s] calloc returned NULL\n", __func__);
    }
    printf("Setting values completed.\n");
}

double gensum(GenMatrix amount[], unsigned short EREE, int loop)
{
    double sum = 0;

    for (int i = 0; i < MAXGEN; i++)
	sum += amount[EREE][i][loop];

    return sum;
}

/* THIS FUNCTION NEEDS UPDATING SO THAT THE USER OF THE INTERFACES INPUTS THE START OF DATA */
void setkey(DataSet *ds)
{
    XLfile *xl = ds->xl;
    char keyCell[32];
    char *row;
    char **xls = xl->sheetname;
    unsigned int i;
    xmlXPathObjectPtr nodeset;
    xmlNodeSetPtr nodes;
    xmlNodePtr node;

    strcpy(ds->keycolumn, "B"); /* This will be changed eventually to use input from user!!! */
    ds->keyrow = 11; /* This will be changed eventually to use input from user!!! */
    strcpy(ds->datasheet, "Data"); /* This will be changed eventually to use input from user!!! */
    for (i = 0; i < xl->sheetcnt; i++)
	if (!strcmp(xls[i], ds->datasheet))
	    break;
    if ((ds->sheet = i) == xl->sheetcnt)
	errExit("[%s] sheet [%s] does not exist\n", __func__, ds->datasheet);

    snprintf(keyCell, sizeof(keyCell), "%s%d", ds->keycolumn, ds->keyrow);

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
	errExit("[%s] key cell [%s] incorrect\n", __func__, keyCell);
}

void countMembers(DataSet *ds)
{
    char *row;
    int r = ds->keyrow;
    int count = ds->keyrow;
    xmlNodePtr node = ds->keynode;

    while (node != NULL && count == r)
    {
	row = (char *)xmlGetProp(node, (xmlChar *)"r");
	r = atoi(row);
	count++;
	r++;
	node = node->next;
    }
    ds->membercnt = count - 1 - ds->keyrow;
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
    ds->keys = (char **)calloc(BUFSIZ/8, sizeof(char *));
    if (ds->keys == NULL) errExit("[%s] calloc returned NULL\n", __func__);
    char **pkey = ds->keys;
    *pkey = cell(ds->xl, ds->sheet, keyCell);

    while (*pkey != NULL)
     {
	// Here we update cell for loop, for example O11 becomes P11
	if (++countkeys >= BUFSIZ/8)
	    errExit("[%s] Data has too many keys\n", __func__);;
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
	// Set the initial data (KEY)
	data = cell(ds->xl, ds->sheet, dataCell);
	lookup(*(ds->keys), data, *(ds->Data + i));
	nextcol(dataCell);
	// Set index of keys to 1 at the start of loop
	countkeys = 1;
	while (*(ds->keys + countkeys) != NULL)
	{

	    while ((data = cell(ds->xl, ds->sheet, dataCell)) == NULL)
	    {
		lookup(*(ds->keys + countkeys), "0", *(ds->Data + i));
		// Here we update cell for loop, for example O11 becomes P11
		countkeys++;
		nextcol(dataCell);
	    }

	    lookup(*(ds->keys + countkeys), data, *(ds->Data + i));

	    // Here we update cell for loop, for example O11 becomes P11
	    countkeys++;
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
    if ((h = lookup(value, NULL, cm->Data)) == NULL)
    {
	printf("warning: '%s' not found in the set of keys given, ", value);
	printf("make sure your column name is correct\n");
	printf("Using 0 by default.\n");
	return strdup("0");
    }
    else
	return h->value;
}

// Example if cm->PREMIUM then s = PREMIUM and we loop through PREMIUM_EREE_GENj
void setGenMatrix(CurrentMember *cm, GenMatrix var[], DataColumn dc)
{
    for (int j = 0; j < MAXGEN; j++)
	for (int EREE = 0; EREE < 2; EREE++)
	    var[EREE][j][0] = atof(getcmval(cm, dc, EREE, j + 1));
}

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
