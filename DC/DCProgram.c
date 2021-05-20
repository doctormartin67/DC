#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
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
    ds->cm = (CurrentMember *)malloc(sizeof(CurrentMember) * ds->membercnt);
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
	cm[i].sal = (double *)malloc(sizeof(double) * MAXPROJ);
	*cm[i].sal = atof(getcmval(&cm[i], "SAL"));
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
	    cm[i].ART24[j][ER][ART24GEN1] = (double *)malloc(sizeof(double) * MAXPROJ);
	    *cm[i].ART24[j][ER][ART24GEN1] = atof(getcmval(&cm[i], "ART24_A_GEN1"));
	    cm[i].ART24[j][ER][ART24GEN2] = (double *)malloc(sizeof(double) * MAXPROJ);
	    *cm[i].ART24[j][ER][ART24GEN2] = atof(getcmval(&cm[i], "ART24_A_GEN2"));
	    cm[i].ART24[j][EE][ART24GEN1] = (double *)malloc(sizeof(double) * MAXPROJ);
	    *cm[i].ART24[j][EE][ART24GEN1] = atof(getcmval(&cm[i], "ART24_C_GEN1"));
	    cm[i].ART24[j][EE][ART24GEN2] = (double *)malloc(sizeof(double) * MAXPROJ);
	    *cm[i].ART24[j][EE][ART24GEN2] = atof(getcmval(&cm[i], "ART24_C_GEN2"));
	}

	// all variables that have generations, employer and employee
	//-  VARIABLES WITH MAXGEN  -
	allocvar(&cm[i], cm[i].PREMIUM, "PREMIUM");
	allocvar(&cm[i], cm[i].CAP, "CAP");
	allocvar(&cm[i], cm[i].CAPPS, "CAPPS");
	allocvar(&cm[i], cm[i].CAPDTH, "CAPDTH");
	for (int k = 0; k < TUCPS_1 + 1; k++) {
	    allocvar(&cm[i], cm[i].RES[k], "RES");
	    allocvar(&cm[i], cm[i].RESPS[k], "RESPS");
	    allocvar(&cm[i], cm[i].REDCAP[k], "CAPRED");
	}
	for (int j = 0; j < MAXGEN; j++) {
	    char tempER[32];
	    char tempEE[32];      
	    snprintf(tempER, sizeof(tempER), "%s%d", "TAUX_A_GEN", j + 1);
	    snprintf(tempEE, sizeof(tempEE), "%s%d", "TAUX_C_GEN", j + 1);
	    cm[i].TAUX[ER][j] = atof(getcmval(&cm[i], tempER));
	    cm[i].TAUX[EE][j] = atof(getcmval(&cm[i], tempEE));      
	    cm[i].RP[ER][j] = (double *)malloc(sizeof(double) * MAXPROJ);
	    cm[i].RP[EE][j] = (double *)malloc(sizeof(double) * MAXPROJ);
	}

	//-  MISCELLANEOUS  -
	cm[i].DELTACAP[ER] = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].DELTACAP[EE] = (double *)malloc(sizeof(double) * MAXPROJ);
	*cm[i].DELTACAP[ER] = atof(getcmval(&cm[i], "DELTA_CAP_A_GEN1"));
	*cm[i].DELTACAP[EE] = atof(getcmval(&cm[i], "DELTA_CAP_C_GEN1"));
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

	// The following get initialised in main loop
	cm[i].DOC = (Date **)malloc(sizeof(Date *) * MAXPROJ);
	cm[i].age = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].nDOE = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].nDOA = (double *)malloc(sizeof(double) * MAXPROJ);

	//---Variables that are used for DBO calculation---
	//***For k = 0 these will all be undefined!!***
	cm[i].FF = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].FFSC = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].qx = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].wxdef = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].wximm = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].retx = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].nPk = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].kPx = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].vk = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].vn = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].vk113 = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].vn113 = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].AFSL = (double *)malloc(sizeof(double) * MAXPROJ);

	for (int k = 0; k < 3; k++) {
	    for (int j = 0; j < 2; j++) {
		cm[i].DBORET[j][k] = (double *)malloc(sizeof(double) * MAXPROJ);
		cm[i].NCRET[j][k] = (double *)malloc(sizeof(double) * MAXPROJ);
		cm[i].ICNCRET[j][k] = (double *)malloc(sizeof(double) * MAXPROJ);		
		for (int l = 0; l < 2; l++) {
		    cm[i].EBP[j][k][l] = (double *)malloc(sizeof(double) * MAXPROJ); 
		}
		cm[i].PBONCCF[j][k] = (double *)malloc(sizeof(double) * MAXPROJ); 
	    }
	    cm[i].assets[k] = (double *)malloc(sizeof(double) * MAXPROJ);
	}
	for (int j = 0; j < 2; j++)
	    cm[i].EBPDTH[j] = (double *)malloc(sizeof(double) * MAXPROJ); 
	cm[i].PBODTHNCCF= (double *)malloc(sizeof(double) * MAXPROJ);

	//---DBO DTH---
	cm[i].CAPDTHRESPart = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].CAPDTHRiskPart = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].DBODTHRESPart = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].DBODTHRiskPart = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].NCDTHRESPart = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].NCDTHRiskPart = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].ICNCDTHRESPart = (double *)malloc(sizeof(double) * MAXPROJ);
	cm[i].ICNCDTHRiskPart = (double *)malloc(sizeof(double) * MAXPROJ);

	//---EXPECTED BENEFITS PAID---


	//***End k = 0 is undefined!!***

    }
    printf("Setting values completed.\n");
}

double gensum(GenPtrArr amount[], unsigned short EREE, int loop) {
    double sum = 0;

    for (int i = 0; i < MAXGEN; i++) {
	sum += amount[EREE][i][loop];
    }
    return sum;
}
/* used to find the row where the keys lie for the data to be used
   for calculations. If the word KEY isn't found in the data then
   1 is returned */
void setkey(DataSet *ds) {
    FILE *fp;
    XLfile *xl;
    char line[BUFSIZ];
    char *begin;
    int value = 1; // value to return
    int i = 0;
    xl = ds->xl;
    while (*(xl->sheetname + i) != NULL) {
	fp = opensheet(xl, *(xl->sheetname + i));

	while (fgets(line, BUFSIZ, fp) != NULL) {
	    begin = line;

	    if ((begin = strcasestr(begin, "KEY")) == NULL)
		continue;
	    if ((begin = strinside(line, "<f>", "</f")) == NULL) {
		printf("ERROR in setkey: awk program didn't manipulate data correctly, ");
		printf("needs checked and updated immediately, exiting program...\n");
		exit(1);
	    }
	    char *temp = begin;
	    char *kc = ds->keycolumn;
	    while (!isdigit(*temp))
		*kc++ = *temp++;
	    *kc = '\0';
	    value = atoi(temp);
	    free(begin);
	    fclose(fp);
	    strcpy(ds->datasheet, *(xl->sheetname + i));
	    ds->keyrow = value;

	    if (strlen(ds->keycolumn) > 3) {
		printf(
			"Error in %s:\n" 
			"the key column: %s has a length larger than 3 which should not be " 
			"possible in excel, exiting program.\n", 
			__func__, ds->keycolumn);
		exit(1);
	    }
	    printf("Found KEY in\nsheet: %s\ncell: %s%d\n",
		    ds->datasheet, ds->keycolumn, ds->keyrow);
	    printf("Setting datasheet to: %s\n", ds->datasheet);
	    printf("Data starts at cell: %s%d\n", ds->keycolumn, ds->keyrow);
	    return;
	}
	fclose(fp);
	i++;
    }
    printf("warning: KEY was not found anywhere, ");
    printf("row 1 is therefore assumed for the key row, ");
    printf("column A is assumed as key column and ");
    printf("\"sheet1\" is assumed as the data sheet name\n");
    strcpy(ds->datasheet, *xl->sheetname);
    ds->keyrow = 1;
    ds->keycolumn[0] = 'A';
}

void countMembers(DataSet *ds) {
    FILE *fp;
    char column[3];
    int irow;
    char srow[16];
    char currentCell[10];
    char *temp;

    fp = opensheet(ds->xl, ds->datasheet);

    strcpy(column, ds->keycolumn);
    irow = ds->keyrow;
    snprintf(srow, sizeof(srow), "%d", irow);
    strcpy(currentCell, column);
    strcat(currentCell, srow);
    while ((temp = cell(fp, currentCell, ds->xl)) != NULL) {
	snprintf(srow, sizeof(srow), "%d", ++irow);
	strcpy(currentCell, column);
	strcat(currentCell, srow);
	free(temp);
    }
    ds->membercnt = irow - 1 - ds->keyrow;
    printf("Amount of affiliates in data: %d\n", ds->membercnt);
    fclose(fp);
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
    ds->Data = (Hashtable **)malloc(ds->membercnt * sizeof(Hashtable *));
    for (int k = 0; k < ds->membercnt; k++) {
	// https://cseweb.ucsd.edu/~kube/cls/100/Lectures/lec16/lec16-8.html
	*(ds->Data + k) = newHashtable(233, 0); // 179 * 1.3 = 232.7 -> 233 is a prime
    }

    // Set the keys
    int countkeys = 0;
    ds->keys = (char **)malloc(BUFSIZ/8 * sizeof(char *));
    *ds->keys = cell(fp, keyCell, ds->xl);

    while (*(ds->keys + countkeys) != NULL || (countkeys > BUFSIZ/8 - 1)) {

	// Here we update cell for loop, for example O11 becomes P11
	countkeys++;
	nextcol(keyCell);
	*(ds->keys + countkeys) = cell(fp, keyCell, ds->xl);
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

	// Set the initial data (KEY)
	data = cell(fp, dataCell, ds->xl);
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

	    while ((data = valueincell(ds->xl, line, dataCell)) == NULL) {

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
    char results[BUFSIZ/4];
    char temp[64]; // to store temporary strings for field names and such.
    int row = 0;
    int col = 0;  

    snprintf(results, sizeof(results), "%s%s", ds->xl->dirname, "/results.xlsx");
    lxw_workbook  *workbook  = workbook_new(results);
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, "Testcases");
    // ***Print Testcases***
    printf("Printing Testcases...\n");
    // at the moment the first member is considered the sole testcase
    //-  Titles of variables  -
    worksheet_write_string(worksheet, row, col, "KEY", NULL);
    worksheet_write_string(worksheet, row, col+1, "DOC", NULL);
    worksheet_write_string(worksheet, row, col+2, "Age", NULL);
    worksheet_write_string(worksheet, row, col+4, "Salary", NULL);
    worksheet_write_string(worksheet, row, col+8, "Contr A", NULL);
    worksheet_write_string(worksheet, row, col+12, "DTH Risk Part", NULL);
    worksheet_write_string(worksheet, row, col+13, "DTH RES Part", NULL);
    worksheet_write_string(worksheet, row, col+14, "Contr C", NULL);

    for (int i = 0; i < MAXGEN; i++) {
	for (int EREE = 0; EREE < EE + 1; EREE++) { 
	    snprintf(temp, sizeof(temp), "CAP GEN %d %c", i + 1, (EREE == ER ? 'A' : 'C'));
	    worksheet_write_string(worksheet, row, col+15 + 4*i + 32*EREE, temp, NULL);
	    snprintf(temp, sizeof(temp), "PREMIUM GEN %d %c", i + 1, (EREE == ER ? 'A' : 'C'));
	    worksheet_write_string(worksheet, row, col+16 + 4*i + 32*EREE, temp, NULL);
	    snprintf(temp, sizeof(temp), "RESERVES PS GEN %d %c", i + 1, (EREE == ER ? 'A' : 'C'));
	    worksheet_write_string(worksheet, row, col+17 + 4*i + 32*EREE, temp, NULL);
	    snprintf(temp, sizeof(temp), "RESERVES GEN %d %c", i + 1, (EREE == ER ? 'A' : 'C'));
	    worksheet_write_string(worksheet, row, col+18 + 4*i + 32*EREE, temp, NULL);
	}
    }

    // Total Reserves
    worksheet_write_string(worksheet, row, col+80, "Total Reserves A", NULL);
    worksheet_write_string(worksheet, row, col+81, "Total Reserves C", NULL);  

    // REDCAP
    worksheet_write_string(worksheet, row, col+82, "RED CAP - PUC", NULL);
    worksheet_write_string(worksheet, row, col+83, "RED CAP - TUC", NULL);
    worksheet_write_string(worksheet, row, col+84, "RED CAP - TUC PS+1", NULL);

    // RESERVES
    worksheet_write_string(worksheet, row, col+85, "RES - PUC", NULL);
    worksheet_write_string(worksheet, row, col+86, "RES - TUC", NULL);
    worksheet_write_string(worksheet, row, col+87, "RES - TUC PS+1", NULL);

    // Article 24
    for (int j = 0; j < TUCPS_1 + 1; j++) {
	for (int i = 0; i < 2; i++) { // generation
	    for (int EREE = 0; EREE < EE + 1; EREE++) { 
		snprintf(temp, sizeof(temp), "ART24 GEN %d %c %s",
			i + 1, (EREE == ER ? 'A' : 'C'),
			(j == PUC ? "PUC" : (j == TUC ? "TUC" : "TUC PS+1")));
		worksheet_write_string(worksheet, row, col+89 + 2*j + i + 6*EREE, temp, NULL);
	    }
	}
    }

    // DBO calculation
    worksheet_write_string(worksheet, row, col+110, "FF", NULL);
    worksheet_write_string(worksheet, row, col+111, "qx", NULL);
    worksheet_write_string(worksheet, row, col+112, "wx (Deferred)", NULL);
    worksheet_write_string(worksheet, row, col+113, "wx (Immediate)", NULL);
    worksheet_write_string(worksheet, row, col+114, "retx", NULL);
    worksheet_write_string(worksheet, row, col+115, "kPx", NULL);
    worksheet_write_string(worksheet, row, col+116, "nPk", NULL);
    worksheet_write_string(worksheet, row, col+117, "v^k", NULL);
    worksheet_write_string(worksheet, row, col+118, "v^n", NULL);

    for (int i = 0; i < 2; i++)
	for (int j = 0; j < 3; j++) {
	    snprintf(temp, sizeof(temp), "DBO RET %s %s",
		    (i == PUC ? "PUC" : "TUC"),
		    (j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
	    worksheet_write_string(worksheet, row, col+119 + j + 3*i, temp, NULL);
	    snprintf(temp, sizeof(temp), "NC RET %s %s",
		    (i == PUC ? "PUC" : "TUC"),
		    (j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
	    worksheet_write_string(worksheet, row, col+125 + j + 3*i, temp, NULL);
	}

    // Assets
    worksheet_write_string(worksheet, row, col+131, "ASSETS PAR 115", NULL);
    worksheet_write_string(worksheet, row, col+132, "ASSETS PAR 113", NULL);
    worksheet_write_string(worksheet, row, col+133, "DBO DTH Risk Part", NULL);
    worksheet_write_string(worksheet, row, col+134, "DBO DTH RES Part", NULL);
    worksheet_write_string(worksheet, row, col+135, "NC DTH Risk Part", NULL);
    worksheet_write_string(worksheet, row, col+136, "NC DTH RES Part", NULL);

    // EBP
    for (int j = 0; j < 3; j++) 
	for (int i = 0; i < 2; i++) {
	    snprintf(temp, sizeof(temp), "PBO NC CF %s %s",
		    (i == PUC ? "PUC" : "TUC"),
		    (j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
	    worksheet_write_string(worksheet, row, col+137 + j + 3*i, temp, NULL);
	    for (int k = 0; k < 2; k++) {
		snprintf(temp, sizeof(temp), "EBP %s %s %s",
			(k == TBO ? "TBO" : "PBO"),
			(i == PUC ? "PUC" : "TUC"),
			(j == PAR115 ? "PAR115" : (j == MATHRES ? "RES" : "PAR113")));
		worksheet_write_string(worksheet, row, col+143 + j + 3*i + 6*k, temp, NULL);
	    }	       
	}
    worksheet_write_string(worksheet, row, col+155, "EBP DTH TBO", NULL);
    worksheet_write_string(worksheet, row, col+156, "EBP DTH PBO", NULL);
    worksheet_write_string(worksheet, row, col+157, "PBO DTH NC CF", NULL);

    //-  Variables  -
    lxw_datetime DOC;
    lxw_format *format = workbook_add_format(workbook);
    char DOCformat[] = "dd/mm/yyyy";
    format_set_num_format(format, DOCformat);
    worksheet_set_column(worksheet, 0, 100, 15, NULL);
    while (row < MAXPROJ) {
	DOC.year = ds->cm[tc].DOC[row]->year;
	DOC.month = ds->cm[tc].DOC[row]->month;
	DOC.day = ds->cm[tc].DOC[row]->day;
	DOC.hour = DOC.min = DOC.sec = 0;
	worksheet_write_string(worksheet, row+1, col, ds->cm[tc].key, NULL);
	worksheet_write_datetime(worksheet, row+1, col+1, &DOC, format);
	worksheet_write_number(worksheet, row+1, col+2, ds->cm[tc].age[row], NULL);
	worksheet_write_number(worksheet, row+1, col+4, ds->cm[tc].sal[row], NULL);
	worksheet_write_number(worksheet, row+1, col+8, gensum(ds->cm[tc].PREMIUM, ER, row), NULL);
	worksheet_write_number(worksheet, row+1, col+12, ds->cm[tc].CAPDTHRiskPart[row], NULL);
	worksheet_write_number(worksheet, row+1, col+13, ds->cm[tc].CAPDTHRESPart[row], NULL);
	worksheet_write_number(worksheet, row+1, col+14, gensum(ds->cm[tc].PREMIUM, EE, row), NULL);
	for (int i = 0; i < MAXGEN; i++) {
	    for (int EREE = 0; EREE < EE + 1; EREE++) { 
		worksheet_write_number(worksheet, row+1, col+15 + 4*i + 32*EREE,
			ds->cm[tc].CAP[EREE][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col+16 + 4*i + 32*EREE,
			ds->cm[tc].PREMIUM[EREE][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col+17 + 4*i + 32*EREE,
			ds->cm[tc].RESPS[PUC][EREE][i][row], NULL);
		worksheet_write_number(worksheet, row+1, col+18 + 4*i + 32*EREE,
			ds->cm[tc].RES[PUC][EREE][i][row], NULL);
	    }
	}
	// Total Reserves
	worksheet_write_number(worksheet, row+1, col+80,
		gensum(ds->cm[tc].RES[PUC], ER, row) +
		gensum(ds->cm[tc].RESPS[PUC], ER, row), NULL);
	worksheet_write_number(worksheet, row+1, col+81,
		gensum(ds->cm[tc].RES[PUC], EE, row) +
		gensum(ds->cm[tc].RESPS[PUC], EE, row), NULL);

	// REDCAP
	worksheet_write_number(worksheet, row+1, col+82,
		gensum(ds->cm[tc].REDCAP[PUC], ER, row) +
		gensum(ds->cm[tc].REDCAP[PUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col+83,
		gensum(ds->cm[tc].REDCAP[TUC], ER, row) +
		gensum(ds->cm[tc].REDCAP[TUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col+84,
		gensum(ds->cm[tc].REDCAP[TUCPS_1], ER, row) +
		gensum(ds->cm[tc].REDCAP[TUCPS_1], EE, row), NULL);

	// RESERVES
	worksheet_write_number(worksheet, row+1, col+85,
		gensum(ds->cm[tc].RES[PUC], ER, row) +
		gensum(ds->cm[tc].RES[PUC], EE, row) +
		gensum(ds->cm[tc].RESPS[PUC], ER, row) +
		gensum(ds->cm[tc].RESPS[PUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col+86,
		gensum(ds->cm[tc].RES[TUC], ER, row) +
		gensum(ds->cm[tc].RES[TUC], EE, row) +
		gensum(ds->cm[tc].RESPS[TUC], ER, row) +
		gensum(ds->cm[tc].RESPS[TUC], EE, row), NULL);
	worksheet_write_number(worksheet, row+1, col+87,
		gensum(ds->cm[tc].RES[TUCPS_1], ER, row) +
		gensum(ds->cm[tc].RES[TUCPS_1], EE, row) +
		gensum(ds->cm[tc].RESPS[TUCPS_1], ER, row) +
		gensum(ds->cm[tc].RESPS[TUCPS_1], EE, row), NULL);

	// Article 24
	for (int j = 0; j < TUCPS_1 + 1; j++) {
	    for (int i = 0; i < 2; i++) { // generation
		for (int EREE = 0; EREE < EE + 1; EREE++) { 
		    worksheet_write_number(worksheet, row+1, col+89 + 2*j + i + 6*EREE,
			    ds->cm[tc].ART24[j][EREE][i][row], NULL);
		}
	    }
	}

	// DBO calculation
	// These variables are shifted one row down because the first row is not used
	if (row+1 < MAXPROJ) {
	    worksheet_write_number(worksheet, row+2, col+110, ds->cm[tc].FF[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+111, ds->cm[tc].qx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+112, ds->cm[tc].wxdef[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+113, ds->cm[tc].wximm[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+114, ds->cm[tc].retx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+115, ds->cm[tc].kPx[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+116, ds->cm[tc].nPk[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+117, ds->cm[tc].vk[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+118, ds->cm[tc].vn[row+1], NULL);

	    for (int i = 0; i < 2; i++)
		for (int j = 0; j < 3; j++) {
		    worksheet_write_number(worksheet,
			    row+2, col+119 + j + 3*i, ds->cm[tc].DBORET[i][j][row+1], NULL);
		    worksheet_write_number(worksheet,
			    row+2, col+125 + j + 3*i, ds->cm[tc].NCRET[i][j][row+1], NULL);
		}

	    // Assets
	    worksheet_write_number(worksheet, row+2, col+131, ds->cm[tc].assets[PAR115][row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+132, ds->cm[tc].assets[PAR113][row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+133, ds->cm[tc].DBODTHRiskPart[row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+134, ds->cm[tc].DBODTHRESPart[row+1] , NULL);
	    worksheet_write_number(worksheet, row+2, col+135, ds->cm[tc].NCDTHRiskPart[row+1] , NULL);
	    worksheet_write_number(worksheet, row+2, col+136, ds->cm[tc].NCDTHRESPart[row+1] , NULL);

	    // EBP
	    for (int j = 0; j < 3; j++) 
		for (int i = 0; i < 2; i++) {
		    worksheet_write_number(worksheet, 
			    row+2, col+137 + j + 3*i, ds->cm[tc].PBONCCF[i][j][row+1], NULL);
		    for (int k = 0; k < 2; k++) {
			worksheet_write_number(worksheet, 
				row+2, col+143 + j + 3*i + 6*k, ds->cm[tc].EBP[i][j][k][row+1], NULL);
		    }	       
		}
	    worksheet_write_number(worksheet, row+2, col+155, ds->cm[tc].EBPDTH[TBO][row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+156, ds->cm[tc].EBPDTH[PBO][row+1], NULL);
	    worksheet_write_number(worksheet, row+2, col+157, ds->cm[tc].PBODTHNCCF[row+1], NULL);
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
	worksheet_write_number(worksheet, row+1, col+7, *ds->cm[row].age, NULL);
	worksheet_write_number(worksheet, row+1, col+8, salaryscale(&ds->cm[row], 1), NULL);
	row++;
    }
    row = 0;

    printf("Printing results complete.\n");
    printf("Printing complete.\n");
    // ***End Print Data***

    return workbook_close(workbook);
}

char *getcmval(CurrentMember *cm, char *value) {
    List *h;
    if ((h = lookup(value, NULL, cm->Data)) == NULL) {
	printf("warning: '%s' not found in the set of keys given, ", value);
	printf("make sure your column name is correct\n");
	printf("Using 0 by default.\n");
	return "0";
    }
    else
	return h->value;
}

// Example if cm->PREMIUM then s = PREMIUM and we loop through PREMIUM_EREE_GENj
void allocvar(CurrentMember *cm, GenPtrArr var[], char *s) {
    char temp[32];

    for (int j = 0; j < MAXGEN; j++) {
	for (int EREE = 0; EREE < 2; EREE++) {
	    snprintf(temp, sizeof(temp), "%s%c%c%s%d",
		    s, '_', (EREE == ER ? 'A' : 'C'), "_GEN", j + 1);
	    var[EREE][j] = (double *)malloc(sizeof(double) * MAXPROJ);
	    *var[EREE][j] = atof(getcmval(cm, temp));
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
