#define _GNU_SOURCE
#include "DCProgram.h"
#include "libraryheader.h"
#include "xlsxwriter.h"

void setDSvals(XLfile *xl, DataSet *ds) {
    ds->xl = xl;
    setkey(ds);
    countMembers(ds);
    createData(ds);
}

// initialise all variables from data (hashtable)
void setCMvals(DataSet *ds) {
    CurrentMember *cm;
    ds->cm = (CurrentMember *)calloc(ds->membercnt, sizeof(CurrentMember));
    cm = ds->cm;
    printf("Setting all the values for the affiliates...\n");
    for (int i = 0; i < ds->membercnt; i++) {
	cm[i].Data = *(ds->Data + i);
	cm[i].key = getcmval(&cm[i], "KEY");
	cm[i].regl = getcmval(&cm[i], "NO REGLEMENT");
	cm[i].name = getcmval(&cm[i], "NAAM");
	cm[i].contract = getcmval(&cm[i], "CONTRACT");
	cm[i].status = 0;
	if (strcmp(getcmval(&cm[i], "STATUS"), "ACT") == 0)  cm[i].status += ACT;
	if (strcmp(getcmval(&cm[i], "ACTIVE CONTRACT"), "1") == 0)  cm[i].status += ACTCON;
	if (strcmp(getcmval(&cm[i], "SEX"), "1") == 0)  cm[i].status += MALE;
	if (strcmp(getcmval(&cm[i], "MS"), "M") == 0)  cm[i].status += MARRIED;
	cm[i].DOB = newDate((unsigned short)atoi(getcmval(&cm[i], "DOB")), 0, 0, 0);
	cm[i].DOE = newDate((unsigned short)atoi(getcmval(&cm[i], "DOE")), 0, 0, 0);
	cm[i].DOL = newDate((unsigned short)atoi(getcmval(&cm[i], "DOL")), 0, 0, 0);
	cm[i].DOS = newDate((unsigned short)atoi(getcmval(&cm[i], "DOS")), 0, 0, 0);
	cm[i].DOA = newDate((unsigned short)atoi(getcmval(&cm[i], "DOA")), 0, 0, 0);
	cm[i].DOR = newDate((unsigned short)atoi(getcmval(&cm[i], "DOR")), 0, 0, 0);
	cm[i].category = getcmval(&cm[i], "CATEGORIE");
	cm[i].sal[0] = atof(getcmval(&cm[i], "SAL"));
	cm[i].PG = atof(getcmval(&cm[i], "PG"));
	cm[i].PT = atof(getcmval(&cm[i], "PT"));
	cm[i].NRA = atof(getcmval(&cm[i], "NRA"));
	cm[i].kids = (unsigned short)atoi(getcmval(&cm[i], "# ENF"));
	cm[i].tariff = 0;
	if (strcmp(getcmval(&cm[i], "TARIEF"), "UKMS") == 0)  cm[i].tariff = UKMS;
	if (strcmp(getcmval(&cm[i], "TARIEF"), "UKZT") == 0)  cm[i].tariff = UKZT;
	if (strcmp(getcmval(&cm[i], "TARIEF"), "UKMT") == 0)  cm[i].tariff = UKMT;
	if (strcmp(getcmval(&cm[i], "TARIEF"), "MIXED") == 0)  cm[i].tariff = MIXED;
	cm[i].KO = atof(getcmval(&cm[i], "KO"));
	cm[i].annINV = atof(getcmval(&cm[i], "Rent INV"));
	cm[i].contrINV = atof(getcmval(&cm[i], "Contr INV"));

	// define article 24 from data
	for (int j = 0; j < TUCPS_1 + 1; j++) {
	    cm[i].ART24[j][ER][ART24GEN1][0] = atof(getcmval(&cm[i], "ART24_A_GEN1"));
	    cm[i].ART24[j][ER][ART24GEN2][0] = atof(getcmval(&cm[i], "ART24_A_GEN2"));
	    cm[i].ART24[j][EE][ART24GEN1][0] = atof(getcmval(&cm[i], "ART24_C_GEN1"));
	    cm[i].ART24[j][EE][ART24GEN2][0] = atof(getcmval(&cm[i], "ART24_C_GEN2"));
	}

	// all variables that have generations, employer and employee
	//-  VARIABLES WITH MAXGEN  -
	setGenMatrix(&cm[i], cm[i].PREMIUM, "PREMIUM");
	setGenMatrix(&cm[i], cm[i].CAP, "CAP");
	setGenMatrix(&cm[i], cm[i].CAPPS, "CAPPS");
	setGenMatrix(&cm[i], cm[i].CAPDTH, "CAPDTH");
	for (int k = 0; k < TUCPS_1 + 1; k++) {
	    setGenMatrix(&cm[i], cm[i].RES[k], "RES");
	    setGenMatrix(&cm[i], cm[i].RESPS[k], "RESPS");
	    setGenMatrix(&cm[i], cm[i].REDCAP[k], "CAPRED");
	}
	for (int j = 0; j < MAXGEN; j++) {
	    char tempER[32];
	    char tempEE[32];      
	    snprintf(tempER, sizeof(tempER), "%s%d", "TAUX_A_GEN", j + 1);
	    snprintf(tempEE, sizeof(tempEE), "%s%d", "TAUX_C_GEN", j + 1);
	    cm[i].TAUX[ER][j] = atof(getcmval(&cm[i], tempER));
	    cm[i].TAUX[EE][j] = atof(getcmval(&cm[i], tempEE));      
	}

	//-  MISCELLANEOUS  -
	cm[i].DELTACAP[ER][0] = atof(getcmval(&cm[i], "DELTA_CAP_A_GEN1"));
	cm[i].DELTACAP[EE][0] = atof(getcmval(&cm[i], "DELTA_CAP_C_GEN1"));
	cm[i].X10 = atof(getcmval(&cm[i], "X/10"));
	if (cm[i].tariff == MIXED && cm[i].X10 == 0) {
	    printf("Warning: X/10 equals zero for %s but he has a MIXED contract\n", cm[i].key);
	    printf("X/10 will be taken as 1 by default.\n");
	    cm[i].X10 = 1;
	}
	cm[i].CAO = atof(getcmval(&cm[i], "CAO"));
	cm[i].ORU = getcmval(&cm[i], "ORU");
	cm[i].CHOICEDTH = getcmval(&cm[i], "CHOICE DTH");
	cm[i].CHOICEINVS = getcmval(&cm[i], "CHOICE INV SICKNESS");
	cm[i].CHOICEINVW = getcmval(&cm[i], "CHOICE INV WORK");
	cm[i].contrDTH = atof(getcmval(&cm[i], "Contr_D"));
	cm[i].percSALKO = atof(getcmval(&cm[i], "%ofSALforKO"));
	cm[i].indexINV = getcmval(&cm[i], "INV INDEXATION");
	cm[i].GRDGR = getcmval(&cm[i], "GR/DGR");
	cm[i].plan = getcmval(&cm[i], "plan");
	cm[i].baranc = atof(getcmval(&cm[i], "Baremische ancienniteit"));
	cm[i].extra = 0;
	if (strcmp(getcmval(&cm[i], "increaseSalFirstYear"), "1") == 0)  cm[i].extra += INCSAL;
	if (strcmp(getcmval(&cm[i], "CCRA"), "1") == 0)  cm[i].extra += CCRA;

	cm[i].DOC = (Date **)calloc(MAXPROJ, sizeof(Date *));
    }
    printf("Setting values completed.\n");
}

double gensum(GenMatrix amount[], unsigned short EREE, int loop) {
    double sum = 0;

    for (int i = 0; i < MAXGEN; i++) {
	sum += amount[EREE][i][loop];
    }
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
	errExit(__func__, "sheet [%s] does not exist\n", ds->datasheet);

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
	errExit(__func__, "key cell [%s] incorrect\n", keyCell);
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

void createData(DataSet *ds) {
    FILE *fp;
    char column[4]; // This holds the column of the beginning of our data cell, for example O
    int irow; // This holds the row of the beginning of our data cell, for example 11
    char srow[16]; /* This holds the row in string form of the beginning of our data cell, 
		      for example "11" */
    char keyCell[10]; // This will hold the cell of a key for the hashtable, for example O11
    char dataCell[10]; /* This will hold the cell of data corresponding to a key
			  for the hashtable, for example O12.*/
    char *data; // This will hold the value of the data, for example 2.391,30.
    char line[BUFSIZ];

    // This opens the data sheet (made with awk).
    fp = opensheet(ds->xl, ds->datasheet);

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

    for (int k = 0; k < ds->membercnt; k++)
	// https://cseweb.ucsd.edu/~kube/cls/100/Lectures/lec16/lec16-8.html
	*(ds->Data + k) = newHashtable(233, 0); // 179 * 1.3 = 232.7 -> 233 is a prime

    // Set the keys
    int countkeys = 0;
    ds->keys = (char **)calloc(BUFSIZ/8, sizeof(char *));
    char **pkey = ds->keys;
    xmlNode offsetnode;
    memset(&offsetnode, 0, sizeof(xmlNode));
    *pkey = cell(ds->xl, ds->sheet, keyCell, &offsetnode);

    while (*pkey != NULL)
     {
	// Here we update cell for loop, for example O11 becomes P11
	if (++countkeys >= BUFSIZ/8)
	    errExit(__func__, "Data has too many keys\n");;
	nextcol(keyCell);
	*++pkey = cell(ds->xl, ds->sheet, keyCell, &offsetnode);
    }

    // Check for double keys
    int k = 0;
    int l = k + 1;
    int cntdouble = 0;
    while (*(ds->keys + k) != NULL) {
	while (*(ds->keys + l) != NULL) {
	    if (strcmp(*(ds->keys + k), *(ds->keys + l)) == 0) {
		char temp[BUFSIZ/256];
		printf("Warning: %s is a double\n", *(ds->keys + l));
		snprintf(temp, sizeof(temp), "%s%d", *(ds->keys + l), ++cntdouble + 1);
		strcpy(*(ds->keys + l), temp);
		printf("Changed it to %s\n",  *(ds->keys + l));
	    }
	    l++;
	}
	l = ++k + 1;
    }

    // start populating Hashtable
    printf("Creating Data...\n");
    for (int i = 0; i < ds->membercnt; i++) {

	memset(&offsetnode, 0, sizeof(xmlNode));
	// Set the initial data (KEY)
	data = cell(ds->xl, ds->sheet, dataCell, &offsetnode);
	lookup(*(ds->keys), data, *(ds->Data + i));
	nextcol(dataCell);
	// Set index of keys to 1 at the start of loop
	countkeys = 1;
	while (*(ds->keys + countkeys) != NULL) {

	    /* get a line from data file, if the data cell we are looking
	       for is not found (== NULL) then there is nothing in the cell 
	       in the excel file. We therefore set the data to 0 and go to
	       next data cell.*/
	    if (fgets(line, sizeof(line), fp) == NULL) {
		printf("Error in %s: should never reach the end of the file before", __func__);
		printf("all the members were created.\n");
		exit(1);
	    }

	    while ((data = cell(ds->xl, ds->sheet, dataCell, &offsetnode)) == NULL) {

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
    fclose(fp);
}

int printresults(DataSet *ds, int tc) {
    char results[PATH_MAX];
    char temp[64]; // to store temporary strings for field names and such.
    int row = 0;
    int col = 0;  

    strcpy(results, ds->xl->dirname);
    strcat(results, "/results.xlsx");
    lxw_workbook  *workbook  = workbook_new(results);
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, "Testcases");
    // ***Print Testcases***
    printf("Printing Testcases...\n");
    // at the moment the first member is considered the sole testcase
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

    for (int i = 0; i < MAXGEN; i++) {
	for (int j = 0; j < 2; j++) {  /* Employer and Employee */
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
    for (int j = 0; j < TUCPS_1 + 1; j++) {
	for (int i = 0; i < 2; i++) { // generation
	    for (int k = 0; k < 2; k++) { /* Employer and Employee */
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
	for (int j = 0; j < 3; j++) {
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
	for (int i = 0; i < 2; i++) {
	    snprintf(temp, sizeof(temp), "PBO NC CF %s %s",
		    (i == PUC ? "PUC" : "TUC"),
		    (j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
	    worksheet_write_string(worksheet, row, col + j + 3*i, temp, NULL);
	    for (int k = 0; k < 2; k++) {
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
    worksheet_set_column(worksheet, 0, 100, 15, NULL);
    CurrentMember *cm = &ds->cm[tc]; // address of test case member

    while (row < MAXPROJ) {
	DOC.year = cm->DOC[row]->year;
	DOC.month = cm->DOC[row]->month;
	DOC.day = cm->DOC[row]->day;
	DOC.hour = DOC.min = DOC.sec = 0;
	worksheet_write_string(worksheet, row+1, col, cm->key, NULL);
	worksheet_write_datetime(worksheet, row+1, col+1, &DOC, format);
	worksheet_write_number(worksheet, row+1, col+2, cm->age[row], NULL);
	worksheet_write_number(worksheet, row+1, col+4, cm->sal[row], NULL);
	worksheet_write_number(worksheet, row+1, col+8, gensum(cm->PREMIUM, ER, row), NULL);
	worksheet_write_number(worksheet, row+1, col+12, cm->CAPDTHRiskPart[row], NULL);
	worksheet_write_number(worksheet, row+1, col+13, cm->CAPDTHRESPart[row], NULL);
	worksheet_write_number(worksheet, row+1, col+14, gensum(cm->PREMIUM, EE, row), NULL);
	for (int i = 0; i < MAXGEN; i++) {
	    for (int j = 0; j < 2; j++) { 
		worksheet_write_number(worksheet, row+1, col+15 + 4*i + 32*j,
			cm->CAP[j][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col+16 + 4*i + 32*j,
			cm->PREMIUM[j][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col+17 + 4*i + 32*j,
			cm->RESPS[PUC][j][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col+18 + 4*i + 32*j,
			cm->RES[PUC][j][i][row], NULL);
	    }
	}
	// Total Reserves
	worksheet_write_number(worksheet, row+1, col+80,
		gensum(cm->RES[PUC], ER, row) +
		gensum(cm->RESPS[PUC], ER, row), NULL);
	worksheet_write_number(worksheet, row+1, col+81,
		gensum(cm->RES[PUC], EE, row) +
		gensum(cm->RESPS[PUC], EE, row), NULL);

	// REDCAP
	worksheet_write_number(worksheet, row+1, col+82,
		gensum(cm->REDCAP[PUC], ER, row) +
		gensum(cm->REDCAP[PUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col+83,
		gensum(cm->REDCAP[TUC], ER, row) +
		gensum(cm->REDCAP[TUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col+84,
		gensum(cm->REDCAP[TUCPS_1], ER, row) +
		gensum(cm->REDCAP[TUCPS_1], EE, row), NULL);

	// RESERVES
	worksheet_write_number(worksheet, row+1, col+85,
		gensum(cm->RES[PUC], ER, row) +
		gensum(cm->RES[PUC], EE, row) +
		gensum(cm->RESPS[PUC], ER, row) +
		gensum(cm->RESPS[PUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col+86,
		gensum(cm->RES[TUC], ER, row) +
		gensum(cm->RES[TUC], EE, row) +
		gensum(cm->RESPS[TUC], ER, row) +
		gensum(cm->RESPS[TUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col+87,
		gensum(cm->RES[TUCPS_1], ER, row) +
		gensum(cm->RES[TUCPS_1], EE, row) +
		gensum(cm->RESPS[TUCPS_1], ER, row) +
		gensum(cm->RESPS[TUCPS_1], EE, row), NULL);

	// Article 24
	for (int j = 0; j < TUCPS_1 + 1; j++) {
	    for (int i = 0; i < 2; i++) { // generation
		for (int k = 0; k < 2; k++) { 
		    worksheet_write_number(worksheet, row+1, col+89 + 2*j + i + 6*k,
			    cm->ART24[j][k][i][row], NULL);
		}
	    }
	}

	// DBO calculation
	// These variables are shifted one row down because the first row is not used
	if (row+1 < MAXPROJ) {
	    worksheet_write_number(worksheet, row+2, col+110, cm->FF[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+111, cm->qx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+112, cm->wxdef[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+113, cm->wximm[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+114, cm->retx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+115, cm->kPx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+116, cm->nPk[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+117, cm->vk[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+118, cm->vn[row+1], NULL);

	    for (int i = 0; i < 2; i++)
		for (int j = 0; j < 3; j++) {
		    worksheet_write_number(worksheet,
			    row+2, col+119 + j + 3*i, cm->DBORET[i][j][row+1], NULL);
		    worksheet_write_number(worksheet,
			    row+2, col+125 + j + 3*i, cm->NCRET[i][j][row+1], NULL);
		}

	    // Assets
	    worksheet_write_number(worksheet, row+2, col+131, cm->assets[PAR115][row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+132, cm->assets[PAR113][row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+133, cm->DBODTHRiskPart[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+134, cm->DBODTHRESPart[row+1] , NULL);
	    worksheet_write_number(worksheet, row+2, col+135, cm->NCDTHRiskPart[row+1] , NULL);
	    worksheet_write_number(worksheet, row+2, col+136, cm->NCDTHRESPart[row+1] , NULL);

	    // EBP
	    for (int j = 0; j < 3; j++) 
		for (int i = 0; i < 2; i++) {
		    worksheet_write_number(worksheet, 
			    row+2, col+137 + j + 3*i, cm->PBONCCF[i][j][row+1], NULL);
		    for (int k = 0; k < 2; k++) {
			worksheet_write_number(worksheet, 
				row+2, col+143 + j + 3*i + 6*k, cm->EBP[i][j][k][row+1], NULL);
		    }	       
		}
	    worksheet_write_number(worksheet, row+2, col+155, cm->EBPDTH[TBO][row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+156, cm->EBPDTH[PBO][row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+157, cm->PBODTHNCCF[row+1], NULL);
	}
	// END SHIFTED VARIABLES

	row++;
    }
    // ***End Print Testcases***
    // ***Print Data***
    row = col = 0;
    worksheet = workbook_add_worksheet(workbook, "dataTY");
    printf("Printing Data...\n");
    while (*(ds->keys + col) != NULL) {
	worksheet_write_string(worksheet, row, col, *(ds->keys + col), NULL);
	while (row < ds->membercnt) {
	    worksheet_write_string(worksheet, row+1, col,
		    lookup(*(ds->keys + col), NULL, *(ds->Data + row))->value, NULL);
	    row++;
	}
	col++;
	row = 0;
    }
    printf("Printing Data complete.\n");
    printf("Printing results...\n");

    worksheet_write_string(worksheet, row, ++col, "DR", NULL);
    worksheet_write_string(worksheet, row, col+1, "DC NC", NULL);
    worksheet_write_string(worksheet, row, col+2, "Method Standard", NULL);
    worksheet_write_string(worksheet, row, col+3, "Method DBO", NULL);
    worksheet_write_string(worksheet, row, col+4, "Method Assets", NULL);
    worksheet_write_string(worksheet, row, col+5, "Method Death", NULL);
    worksheet_write_string(worksheet, row, col+6, "Admin Cost", NULL);
    worksheet_write_string(worksheet, row, col+7, "Age", NULL);
    worksheet_write_string(worksheet, row, col+8, "Salary Scale", NULL);
    while (row < ds->membercnt) {
	worksheet_write_number(worksheet, row+1, col, ass.DR, NULL);
	worksheet_write_number(worksheet, row+1, col+1, ass.DR, NULL);
	worksheet_write_string(worksheet, row+1, col+2, 
		(ass.method & mIAS ? "IAS" : "FAS"), NULL);
	worksheet_write_string(worksheet, row+1, col+3, 
		(ass.method & mTUC ? "TUC" : "PUC"), NULL);
	worksheet_write_string(worksheet, row+1, col+4, 
		(ass.method & mRES ? "RES" : 
		 (ass.method & mPAR115 ? "PAR115" : "PAR113")), NULL);
	worksheet_write_number(worksheet, row+1, col+5, 
		(ass.method & mDTH ? 1 : 0), NULL);
	worksheet_write_number(worksheet, row+1, col+6, tff.admincost, NULL);
	worksheet_write_number(worksheet, row+1, col+7, *cm[row].age, NULL);
	worksheet_write_number(worksheet, row+1, col+8, salaryscale(&cm[row], 1), NULL);
	row++;
    }
    row = 0;

    printf("Printing results complete.\n");
    printf("Printing complete.\n");
    // ***End Print Data***

    return workbook_close(workbook);
}

char *getcmval(CurrentMember *cm, const char *value) {
    List *h;
    if ((h = lookup(value, NULL, cm->Data)) == NULL) {
	printf("warning: '%s' not found in the set of keys given, ", value);
	printf("make sure your column name is correct\n");
	printf("Using 0 by default.\n");
	return strdup("0");
    }
    else
	return h->value;
}

// Example if cm->PREMIUM then s = PREMIUM and we loop through PREMIUM_EREE_GENj
void setGenMatrix(CurrentMember *cm, GenMatrix var[], char *s) {
    char temp[32];

    for (int j = 0; j < MAXGEN; j++) {
	for (int EREE = 0; EREE < 2; EREE++) {
	    snprintf(temp, sizeof(temp), "%s%c%c%s%d",
		    s, '_', (EREE == ER ? 'A' : 'C'), "_GEN", j + 1);
	    var[EREE][j][0] = atof(getcmval(cm, temp));
	}
    }
}

double salaryscale(CurrentMember *cm, int k) {
    return (*ass.SS)(cm, k);
}

double calcA(CurrentMember *cm, int k) {
    return (*ass.calcA)(cm, k);
}

double calcC(CurrentMember *cm, int k) {
    return (*ass.calcC)(cm, k);
}

double calcDTH(CurrentMember *cm, int k) {
    return (*ass.calcDTH)(cm, k);
}

double NRA(CurrentMember *cm, int k) {
    return (*ass.NRA)(cm, k);
}

double wxdef(CurrentMember *cm, int k) {
    return (*ass.wxdef)(cm, k);
}

double retx(CurrentMember *cm, int k) {
    return (*ass.retx)(cm, k);
}
