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
      cm[i].ART24[ER][ART24GEN1][j] = (double *)malloc(sizeof(double) * MAXPROJ);
      *cm[i].ART24[ER][ART24GEN1][j] = atof(getcmval(&cm[i], "ART24_A_GEN1"));
      cm[i].ART24[ER][ART24GEN2][j] = (double *)malloc(sizeof(double) * MAXPROJ);
      *cm[i].ART24[ER][ART24GEN2][j] = atof(getcmval(&cm[i], "ART24_A_GEN2"));
      cm[i].ART24[EE][ART24GEN1][j] = (double *)malloc(sizeof(double) * MAXPROJ);
      *cm[i].ART24[EE][ART24GEN1][j] = atof(getcmval(&cm[i], "ART24_C_GEN1"));
      cm[i].ART24[EE][ART24GEN2][j] = (double *)malloc(sizeof(double) * MAXPROJ);
      *cm[i].ART24[EE][ART24GEN2][j] = atof(getcmval(&cm[i], "ART24_C_GEN2"));
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
      memset(tempER, '\0', sizeof(tempER));
      memset(tempEE, '\0', sizeof(tempEE));
      cm[i].RP[ER][j] = (double *)malloc(sizeof(double) * MAXPROJ);
      cm[i].RP[EE][j] = (double *)malloc(sizeof(double) * MAXPROJ);
    }

    //-  MISCELLANEOUS  -
    cm[i].DELTACAP[ER] = (double *)malloc(sizeof(double) * MAXPROJ);
    cm[i].DELTACAP[EE] = (double *)malloc(sizeof(double) * MAXPROJ);
    *cm[i].DELTACAP[ER] = atof(getcmval(&cm[i], "DELTA_CAP_A_GEN1"));
    *cm[i].DELTACAP[EE] = atof(getcmval(&cm[i], "DELTA_CAP_C_GEN1"));
    cm[i].X10 = atof(getcmval(&cm[i], "X/10"));
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
  }
  printf("Setting values completed.\n");
}

double gensum(double *amount[][MAXGEN], unsigned short EREE, int loop) {
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
  char sname[BUFSIZ]; // name of the sheet to open (xml file)
  char *begin;
  int sheetnr;
  int value = 1; // value to return
  int i,j = 0;
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
      while (!isdigit(*temp)) {
	ds->keycolumn[j++] = *temp;
	temp++;
      }
      value = atoi(temp);
      free(begin);
      fclose(fp);
      strcpy(ds->datasheet, *(xl->sheetname + i));
      ds->keyrow = value;
      
      if (strlen(ds->keycolumn) > 3) {
	printf("the key column: %s has a length larger than 3 which should not be ",
	       ds->keycolumn);
	printf("possible in excel, exiting program.\n");
	exit(1);
      }
      printf("Found KEY in\nsheet: %s\ncell: %s%d\n",
	     ds->datasheet, ds->keycolumn, ds->keyrow);
      printf("Setting datasheet to: %s\n", ds->datasheet);
      printf("Data starts at cell: %s%d\n", ds->keycolumn, ds->keyrow);
      return;
    }
    i++;
  }
  fclose(fp);
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
  int count = 0;
  char column[3];
  int irow;
  char srow[6];
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
  char srow[6]; /* This holds the row in string form of the beginning of our data cell, 
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
  memset(srow, '\0', sizeof(srow));
  snprintf(srow, sizeof(srow), "%d", irow);
  strcpy(dataCell, column);
  strcat(dataCell, srow);
  
  // Allocate memory for data matrix and initialise to NULL pointer
  ds->Data = (Hashtable ***)malloc(ds->membercnt * sizeof(Hashtable **));
  for (int k = 0; k < ds->membercnt; k++) {
    *(ds->Data + k) = (Hashtable **)malloc(HASHSIZE * sizeof(Hashtable *));
    for (int l = 0; l < HASHSIZE; l++)
      *(*(ds->Data + k) + l) = NULL;
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
	memset(*(ds->keys + l), '\0', sizeof(*(ds->keys + l)));
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
    set(*(ds->keys), data, *(ds->Data + i));
    nextcol(dataCell);
    // Set index of keys to 1 at the start of loop
    countkeys = 1;
    while (*(ds->keys + countkeys) != NULL) {

      /* get a line from data file, if the data cell we are looking
       for is not found (== NULL) then there is nothing in the cell 
       in the excel file. We therefore set the data to 0 and go to
       next data cell.*/
      fgets(line, sizeof(line), fp);
      while ((data = valueincell(ds->xl, line, dataCell)) == NULL) {

	set(*(ds->keys + countkeys), "0", *(ds->Data + i));
	// Here we update cell for loop, for example O11 becomes P11
	countkeys++;
	nextcol(dataCell);
      }
      
      set(*(ds->keys + countkeys), data, *(ds->Data + i));
      
      // Here we update cell for loop, for example O11 becomes P11
      countkeys++;
      nextcol(dataCell);
    }

    // Iterate through all affiliates by updating dataCell
    irow++;
    memset(srow, '\0', sizeof(srow));
    memset(dataCell, '\0', sizeof(dataCell));
    snprintf(srow, sizeof(srow), "%d", irow);
    strcpy(dataCell, column);
    strcat(dataCell, srow);
    
  }
  printf("Creation complete\n");
  fclose(fp);
}

int printresults(DataSet *ds) {
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
  for (int i = 0; i < MAXGEN; i++) {
    for (int EREE = 0; EREE < EE + 1; EREE++) { 
      snprintf(temp, sizeof(temp), "CAP GEN %d %c", i + 1, (EREE == ER ? 'A' : 'C'));
      worksheet_write_string(worksheet, row, col+15 + 4*i + 32*EREE, temp, NULL);
      memset(temp, '\0', sizeof(temp));
      snprintf(temp, sizeof(temp), "PREMIUM GEN %d %c", i + 1, (EREE == ER ? 'A' : 'C'));
      worksheet_write_string(worksheet, row, col+16 + 4*i + 32*EREE, temp, NULL);
      memset(temp, '\0', sizeof(temp));
      snprintf(temp, sizeof(temp), "RESERVES PS GEN %d %c", i + 1, (EREE == ER ? 'A' : 'C'));
      worksheet_write_string(worksheet, row, col+17 + 4*i + 32*EREE, temp, NULL);
      memset(temp, '\0', sizeof(temp));
      snprintf(temp, sizeof(temp), "RESERVES GEN %d %c", i + 1, (EREE == ER ? 'A' : 'C'));
      worksheet_write_string(worksheet, row, col+18 + 4*i + 32*EREE, temp, NULL);
      memset(temp, '\0', sizeof(temp));
    }
  }
  //-  Variables  -
  lxw_datetime DOC;
  lxw_format *format = workbook_add_format(workbook);
  char DOCformat[] = "dd/mm/yyyy";
  format_set_num_format(format, DOCformat);
  worksheet_set_column(worksheet, 0, 100, 15, NULL);
  while (row < MAXPROJ) {
    DOC.year = ds->cm[0].DOC[row]->year;
    DOC.month = ds->cm[0].DOC[row]->month;
    DOC.day = ds->cm[0].DOC[row]->day;
    DOC.hour = DOC.min = DOC.sec = 0;
    worksheet_write_string(worksheet, row+1, col, ds->cm[0].key, NULL);
    worksheet_write_datetime(worksheet, row+1, col+1, &DOC, format);
    worksheet_write_number(worksheet, row+1, col+2, ds->cm[0].age[row], NULL);
    worksheet_write_number(worksheet, row+1, col+4, ds->cm[0].sal[row], NULL);
    worksheet_write_number(worksheet, row+1, col+8, gensum(ds->cm[0].PREMIUM, ER, row), NULL);
    for (int i = 0; i < MAXGEN; i++) {
      for (int EREE = 0; EREE < EE + 1; EREE++) { 
	worksheet_write_number(worksheet, row+1, col+15 + 4*i + 32*EREE,
			       ds->cm[0].CAP[EREE][i][row], NULL);
	worksheet_write_number(worksheet, row+1, col+16 + 4*i + 32*EREE,
			       ds->cm[0].PREMIUM[EREE][i][row], NULL);
	worksheet_write_number(worksheet, row+1, col+17 + 4*i + 32*EREE,
			       ds->cm[0].RESPS[PUC][EREE][i][row], NULL);
	worksheet_write_number(worksheet, row+1, col+18 + 4*i + 32*EREE,
			       ds->cm[0].RES[PUC][EREE][i][row], NULL);
      }
    }
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
			     get(*(ds->keys + col), *(ds->Data + row))->value, NULL);
      row++;
    }
    col++;
    row = 0;
  }
  printf("Printing Data complete.\n");
  printf("Printing results...\n");
  col+=5;
  worksheet_write_string(worksheet, row, col, "Age", NULL);
  while (row < ds->membercnt) {
    worksheet_write_number(worksheet, row+1, col, *ds->cm[row].age, NULL);
    row++;
  }
  col++;
  row = 0;
  
  printf("Printing results complete.\n");
  printf("Printing complete.\n");
  // ***End Print Data***
  return workbook_close(workbook);
}

char *getcmval(CurrentMember *cm, char *value) {
  if (get(value, cm->Data) == NULL) {
    printf("warning: '%s' not found in the set of keys given, ", value);
    printf("make sure your column name is correct\n");
    printf("Using 0 by default.\n");
    return "0";
  }
  else
    return get(value, cm->Data)->value;
}

// Example if cm->PREMIUM then s = PREMIUM and we loop through PREMIUM_EREE_GENj
void allocvar(CurrentMember *cm, double *var[][MAXGEN], char *s) {
  char temp[32];
  
  for (int j = 0; j < MAXGEN; j++) {
    for (int EREE = 0; EREE < 2; EREE++) {
      snprintf(temp, sizeof(temp), "%s%c%c%s%d",
	       s, '_', (EREE == ER ? 'A' : 'C'), "_GEN", j + 1);
      var[EREE][j] = (double *)malloc(sizeof(double) * MAXPROJ);
      *var[EREE][j] = atof(getcmval(cm, temp));
      memset(temp, '\0', sizeof(temp));
    }
  }
}

double salaryscale(CurrentMember *cm, int k) {
  return (*ass.SS)(cm, k);
}

double NRA(CurrentMember *cm, int k) {
  return (*ass.NRA)(cm, k);
}

double wxdef(CurrentMember *cm, int k) {
  return (*ass.wxdef)(cm, k);
}

double wximm(CurrentMember *cm, int k) {
  return (*ass.wximm)(cm, k);
}

double calcA(CurrentMember *cm, int k) {
  return (*ass.calcA)(cm, k);
}

double calcC(CurrentMember *cm, int k) {
  return (*ass.calcC)(cm, k);
}
