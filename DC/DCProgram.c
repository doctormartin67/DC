#define _GNU_SOURCE
#include "libraryheader.h"
#include "errorexit.h"
#include "XL.h"
#include "assumptions.h"

static void clean_double_keys(char *keys[static 1]);
static unsigned countdigits(unsigned d);

const char *const colnames[KEYS_AMOUNT] = {
	[KEY] = "KEY", [NOREGLEMENT] = "NO REGLEMENT", [NAAM] = "NAAM",
	[CONTRACT] = "CONTRACT", [STATUS] = "STATUS",
	[ACTIVECONTRACT] = "ACTIVE CONTRACT", [SEX] = "SEX", [MS] = "MS",
	[DOB] = "DOB", [DOE] = "DOE", [DOL] = "DOL", [DOS] = "DOS",
	[DOA] = "DOA", [DOR] = "DOR", [CATEGORIE] = "CATEGORIE", [SAL] = "SAL",
	[PG] = "PG", [PT] = "PT", [NORMRA] = "NRA", [ENF] = "# ENF",
	[TARIEF] = "TARIEF", [KO] = "KO", [RENTINV] = "Rent INV",
	[CONTRINV] = "Contr INV", [ART24_A_GEN1] = "ART24_A_GEN1", 
	[ART24_A_GEN2] = "ART24_A_GEN2", [ART24_C_GEN1] = "ART24_C_GEN1",
	[ART24_C_GEN2] = "ART24_C_GEN2", [PREMIUM] = "PREMIUM", [CAP] = "CAP",
	[CAPPS] = "CAPPS", [CAPDTH] = "CAPDTH", [RES] = "RES",
	[RESPS] = "RESPS", [CAPRED] = "CAPRED", [TAUX] = "TAUX",
	[DELTA_CAP_A_GEN1] = "DELTA_CAP_A_GEN1",
	[DELTA_CAP_C_GEN1] = "DELTA_CAP_C_GEN1", [X10] = "X/10", [CAO] = "CAO",
	[ORU] = "ORU", [CHOICEDTH] = "CHOICE DTH",
	[CHOICEINVS] = "CHOICE INV SICKNESS", [CHOICEINVW] = "CHOICE INV WORK",
	[CONTRD] = "Contr_D", [PERCOFSALFORKO] = "%ofSALforKO",
	[INVINDEXATION] = "INV INDEXATION", [GRDGR] = "GR/DGR",
	[PLAN] = "plan", [BARANC] = "Baremische ancienniteit",
	[INCSALFIRSTYEAR] = "increaseSalFirstYear", [PREP] = "CCRA"
};

const char *const inscomb[INSCOMB_AMOUNT] = {
	[UKMS] = "UKMS", [UKZT] = "UKZT", [UKMT] = "UKMT", [MIXED] = "MIXED"
};

static int colmissing[KEYS_AMOUNT];

static Validator *val;

DataSet *createDS(Validator v[static 1], Hashtable ht_ui[static 1])
{
	const char *s = ht_get(get_ui_key(SPECIAL_FILENAME, UI_SPECIAL),ht_ui);
	Hashtable **ht = 0;
	DataSet *ds = jalloc(1, sizeof(*ds));
	*ds = (DataSet){0};

	for (unsigned i = 0; i < KEYS_AMOUNT; i++)
		colmissing[i] = 0;

	val = v;

	createXLzip(s); /* THIS IS NOT PORTABLE!! */

	XLfile *xl = createXL(s);

	ds->xl = xl;
	ds->ht_user_input = ht_ui;

	if (0 == ds->xl) {
		updateValidation(val, ERROR, "Unable to parse excel file [%s],"
				"is the file empty?", s);
		freeDS(ds);
		return 0;
	}

	if (!setkey(ds)) {
		freeDS(ds);
		return 0;
	}

	countMembers(ds);
	createData(ds);

	ht = ds->Data;
	ds->cm = jalloc(ds->membercnt, sizeof(*ds->cm));

	printf("Setting all the values for the affiliates...\n");
	for (unsigned i = 0; i < ds->membercnt; i++) {
		ds->cm[i] = (CurrentMember){0};
		ds->cm[i].id = i + 1;
		createCM(&ds->cm[i], *ht++);
	}
	printf("Setting values completed.\n");

	validateColumns();

	return ds;
}

// initialise all variables from data (hashtable)
void createCM(CurrentMember cm[static 1], Hashtable ht[static 1])
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
	cm->DOB = newDate((unsigned)atoi(getcmval(cm, DOB, -1, -1)), 0, 0, 0);
	cm->DOE = newDate((unsigned)atoi(getcmval(cm, DOE, -1, -1)), 0, 0, 0);
	cm->DOL = newDate((unsigned)atoi(getcmval(cm, DOL, -1, -1)), 0, 0, 0);
	cm->DOS = newDate((unsigned)atoi(getcmval(cm, DOS, -1, -1)), 0, 0, 0);
	cm->DOA = newDate((unsigned)atoi(getcmval(cm, DOA, -1, -1)), 0, 0, 0);
	cm->DOR = newDate((unsigned)atoi(getcmval(cm, DOR, -1, -1)), 0, 0, 0);
	cm->category = getcmval(cm, CATEGORIE, -1, -1);
	cm->sal[0] = atof(getcmval(cm, SAL, -1, -1));
	cm->PG = atof(getcmval(cm, PG, -1, -1));
	cm->PT = atof(getcmval(cm, PT, -1, -1));
	cm->NRA = atof(getcmval(cm, NORMRA, -1, -1));
	cm->kids = (unsigned)atoi(getcmval(cm, ENF, -1, -1));
	cm->tariff = 0;
	for (unsigned i = 0; i < INSCOMB_AMOUNT; i++)
		if (strcmp(getcmval(cm, TARIEF, -1, -1), inscomb[i]) == 0)
			cm->tariff = i;
	cm->KO = atof(getcmval(cm, KO, -1, -1));
	cm->annINV = atof(getcmval(cm, RENTINV, -1, -1));
	cm->contrINV = atof(getcmval(cm, CONTRINV, -1, -1));

	// define article 24 from data
	cm->ART24[PUC][ER][ART24GEN1][0] = atof(getcmval(cm, ART24_A_GEN1, -1, -1));
	cm->ART24[PUC][ER][ART24GEN2][0] = atof(getcmval(cm, ART24_A_GEN2, -1, -1));
	cm->ART24[PUC][EE][ART24GEN1][0] = atof(getcmval(cm, ART24_C_GEN1, -1, -1));
	cm->ART24[PUC][EE][ART24GEN2][0] = atof(getcmval(cm, ART24_C_GEN2, -1, -1));
	/* PUC = TUC = TUCPS_1 */
	for (unsigned j = 1; j < METHOD_AMOUNT; j++) {
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
	for (unsigned k = 0; k < METHOD_AMOUNT; k++) {
		setGenMatrix(cm, cm->RES[k], RES);
		setGenMatrix(cm, cm->RESPS[k], RESPS);
		setGenMatrix(cm, cm->REDCAP[k], CAPRED);
	}
	for (unsigned j = 0; j < MAXGEN; j++) {
		cm->TAUX[ER][j] = atof(getcmval(cm, TAUX, ER, j + 1));
		cm->TAUX[EE][j] = atof(getcmval(cm, TAUX, EE, j + 1));      
	}

	//-  MISCELLANEOUS  -
	cm->DELTACAP[ER][0] = atof(getcmval(cm, DELTA_CAP_A_GEN1, -1, -1));
	cm->DELTACAP[EE][0] = atof(getcmval(cm, DELTA_CAP_C_GEN1, -1, -1));
	cm->X10 = atof(getcmval(cm, X10, -1, -1));
	if (cm->tariff == MIXED && cm->X10 == 0) {
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
}

double gensum(const GenMatrix amount[static EREE_AMOUNT],
		unsigned EREE, unsigned k)
{
	double sum = 0;

	for (unsigned i = 0; i < MAXGEN; i++) sum += amount[EREE][i][k];

	return sum;
}

int setkey(DataSet ds[static 1])
{
	XLfile *xl = ds->xl;
	char *c = 0;
	const char *kc = 0;
	char *row = 0; 
	char **xls = xl->sheetname;
	unsigned i = 0;
	xmlXPathObjectPtr nodeset = 0;
	xmlNodeSetPtr nodes = 0;
	xmlNodePtr node = 0;

	kc = ht_get(get_ui_key(SPECIAL_KEYCELL, UI_SPECIAL),ds->ht_user_input);
	setcol(ds->keycolumn, kc);
	ds->keyrow = getrow(kc);

	snprintf(ds->datasheet, sizeof(ds->datasheet), 
			"%s", ht_get("sheetname", ds->ht_user_input));
	for (i = 0; i < xl->sheetcnt; i++)
		if (!strcmp(xls[i], ds->datasheet))
			break;
	if ((ds->sheet = i) == xl->sheetcnt) {
		updateValidation(val, ERROR, "Sheet [%s] does not exist",
				ds->datasheet);
		return 0;
	} else {
		/* Check whether cell that was provided has anything in it */
		if (0 == (c = cell(ds->xl, ds->sheet, kc))) {
			updateValidation(val, ERROR, 
					"Nothing in cell [%s] found", kc);
			return 0;
		}
		free(c);

		nodeset = xl->nodesets[ds->sheet];
		nodes = nodeset->nodesetval;

		for (node = *nodes->nodeTab; 0 != node; node = node->next) {
			row = (char *)xmlGetProp(node, (const xmlChar *)"r");
			if ((unsigned)atoi(row) == ds->keyrow) {
				xmlFree(row);
				break;
			} else {
				xmlFree(row);
			}
		}

		if (0 == (ds->keynode = node)) {
			updateValidation(val, ERROR, 
					"Nothing in cell [%s] found", kc);
			return 0;
		} else
			return 1;
	}
}

void countMembers(DataSet ds[static 1])
{
	char *row = 0;
	int r = ds->keyrow;
	int count = ds->keyrow;
	xmlNodePtr node = ds->keynode;

	while (0 != node) {
		row = (char *)xmlGetProp(node, (xmlChar *)"r");
		r = atoi(row);

		/* cells below the data should not be considered */
		if (r - count == 1) count++;
		node = node->next;
		xmlFree(row);
	}
	ds->membercnt = count - ds->keyrow;
	printf("Amount of affiliates in data: %d\n", ds->membercnt);
}

/* Excel is just a bunch of cells of the form O11, DC103, ...
   This function will create a pointer to an array of arrays of pointers to 
   hashtables, where we will store all the cells for our data. We will 
   reference cell O11 as an example going further.*/

void createData(DataSet ds[static 1])
{
	unsigned countkeys = 0;
	unsigned irow = ds->keyrow;
	char column[4];
	char keyCell[10];
	char dataCell[10];
	char *data = 0;
	char **pkey = 0;

	// Here we set the initial key cell, for example B11
	snprintf(column, sizeof(column), "%s", ds->keycolumn);
	snprintf(keyCell, sizeof(keyCell), "%s%u", column, irow);

	// Here we set the initial data cell, for example B12
	irow++;
	snprintf(dataCell, sizeof(dataCell), "%s%u", column, irow);

	ds->Data = jalloc(ds->membercnt, sizeof(*ds->Data));

	for (unsigned k = 0; k < ds->membercnt; k++)
		// https://cseweb.ucsd.edu/~kube/cls/100/Lectures/lec16/lec16-8.html
		ds->Data[k] = newHashtable(233, 0); // 179 * 1.3 = 232.7 -> 233 is a prime

	ds->keys = jalloc(KEYS_AMOUNT, sizeof(*ds->keys));
	pkey = ds->keys;
	*pkey = cell(ds->xl, ds->sheet, keyCell);

	while (0 != *pkey) {
		// Here we update cell for loop, for example O11 becomes P11
		if (++countkeys >= KEYS_AMOUNT)
			die("Data has too many keys");
		nextcol(keyCell);
		*++pkey = cell(ds->xl, ds->sheet, keyCell);
	}

	clean_double_keys(ds->keys);

	// start populating Hashtable
	printf("Creating Data...\n");
	for (unsigned i = 0; i < ds->membercnt; i++) {
		pkey = ds->keys;
		// Set the initial data (KEY)
		if (0 == (data = cell(ds->xl, ds->sheet, dataCell))) {
			ht_set(*pkey++, "0", *(ds->Data + i));
		} else {
			ht_set(*pkey++, data, *(ds->Data + i));
			free(data);
		}
		nextcol(dataCell);
		while (0 != *pkey) {
			while (0 == (data = cell(ds->xl, ds->sheet, dataCell))) {
				if (0 != *pkey)
					ht_set(*pkey++, "0", *(ds->Data + i));
				else
					break;
				// Here we update cell for loop, for example O11 becomes P11
				nextcol(dataCell);
			}

			if (0 != *pkey) {
				ht_set(*pkey++, data, *(ds->Data + i));
				free(data);
			} else {
				free(data);
				break;
			}
			// Here we update cell for loop, for example O11 becomes P11
			nextcol(dataCell);
		}

		irow++;
		snprintf(dataCell, sizeof(dataCell), "%s%u", column, irow);
	}
	printf("Creation complete\n");
}

static void clean_double_keys(char *keys[static 1])
{
	size_t len = 0;
	unsigned cnt = 1;
	char *newkey = 0;
	char **pkey = keys + 1;
	while (0 != *keys) {
		while (0 != *pkey) {
			if (0 == strcmp(*keys, *pkey)) {
				printf("Warning: %s is a double\n", *pkey);
				cnt++;
				len = strlen(*pkey) + countdigits(cnt) + 1;
				newkey = jalloc(len, sizeof(*newkey));
				snprintf(newkey, len, "%s%u", *pkey, cnt);
				free(*pkey);
				*pkey = newkey;
				printf("Changed it to %s\n",  *pkey);
			}
			pkey++;
		}
		pkey = ++keys + 1;
		cnt = 0;
	}
}

static unsigned countdigits(unsigned d)
{
	unsigned cnt = 1;

	while (d/10) cnt++;

	return cnt;
}

const char *getcmval(CurrentMember cm[static 1], DataColumn dc, int EREE,
		int gen)
{
	char value[BUFSIZ];
	const char *h = 0;

	if (EREE >= 0 && gen > 0)
		snprintf(value, sizeof(value), "%s%c%c%s%d", colnames[dc], '_',
				(EREE == ER ? 'A' : 'C'), "_GEN", gen);
	else
		snprintf(value, sizeof(value), "%s", colnames[dc]);

	if (colmissing[dc] || 0 == (h = ht_get(value, cm->Data))) {
		colmissing[dc] = 1;
		return strdup("0");
	} else {
		validateInput(dc, cm, value, h);
		return h;
	}
}

// Example if cm->PREMIUM then s = PREMIUM and we loop through PREMIUM_EREE_GENj
void setGenMatrix(CurrentMember cm[static 1],
		GenMatrix var[static EREE_AMOUNT], DataColumn dc)
{
	for (int j = 0; j < MAXGEN; j++)
		for (int i = 0; i < EREE_AMOUNT; i++)
			var[i][j][0] = atof(getcmval(cm, dc, i, j + 1));
}

/* --- free memory functions --- */
void freeDS(DataSet *ds)
{
	if (0 == ds) return;

	char **s = ds->keys;
	while (0 != s && 0 != *s)
		free(*s++);
	free(ds->keys);

	for (unsigned i = 0; i < ds->membercnt; i++)
		freeCM(&ds->cm[i]);
	free(ds->cm);

	freeXL(ds->xl);

	for (unsigned i = 0; i < ds->membercnt; i++)
		freeHashtable(ds->Data[i]);
	free(ds->Data);
	free(ds);
}

void freeCM(CurrentMember *cm)
{
	if (0 == cm) return;

	free(cm->DOB);
	free(cm->DOE);
	free(cm->DOL);
	free(cm->DOS);
	free(cm->DOA);
	free(cm->DOR);
	for (unsigned i = 0; i < MAXPROJ + 1; i++)
		free(cm->DOC[i]);
}

/* 
 * All missing columns are set to a WARNING,
 * then the important ones are set to ERROR
 */
void validateColumns()
{
	unsigned cnt = 0;

	for (unsigned i = 0; i < KEYS_AMOUNT; i++)
		if (colmissing[i])
			updateValidation(val, WARNING, "Column [%s] missing, "
					"all values set to 0", colnames[i]);

	if (colmissing[STATUS]) {
		updateValidation(val, ERROR, "Column [%s] missing, (ACT/DEF)",
				colnames[STATUS]);
		cnt++;
	}

	if (colmissing[SEX]) {
		updateValidation(val, ERROR, "Column [%s] missing, (1/2)",
				colnames[SEX]);
		cnt++;
	}

	if (colmissing[DOB]) {
		updateValidation(val, ERROR, "Column [%s] missing, "
				"(Date of birth)", colnames[DOB]);
		cnt++;
	}

	if (colmissing[DOS]) {
		updateValidation(val, ERROR, "Column [%s] missing, "
				"(Date of situation)", colnames[DOS]);
		cnt++;
	}

	if (colmissing[DOA]) {
		updateValidation(val, ERROR, "Column [%s] missing, "
				"(Date of affiliation)", colnames[DOA]);
		cnt++;
	}

	if (colmissing[DOR]) {
		updateValidation(val, ERROR, "Column [%s] missing, "
				"(Date of retirement)", colnames[DOR]);
		cnt++;
	}

	if (colmissing[SAL]) {
		updateValidation(val, ERROR, "Column [%s] missing, (salary)",
				colnames[SAL]);
		cnt++;
	}

	if (colmissing[PT]) {
		updateValidation(val, ERROR, "Column [%s] missing, "
				"(part time)", colnames[PT]);
		cnt++;
	}

	if (colmissing[NORMRA]) {
		updateValidation(val, ERROR, "Column [%s] missing, "
				"(normal retirement age)", colnames[NORMRA]);
		cnt++;
	}

	if (colmissing[TARIEF]) {
		updateValidation(val, ERROR, "Column [%s] missing, "
				"(UKMS/UKZT/MIXED/UKMT)", colnames[TARIEF]);
		cnt++;
	}

	if (colmissing[ART24_A_GEN1]) {
		updateValidation(val, ERROR, "Column [%s] missing",
				colnames[ART24_A_GEN1]);
		cnt++;
	}

	if (colmissing[ART24_A_GEN2]) {
		updateValidation(val, ERROR, "Column [%s] missing",
				colnames[ART24_A_GEN2]);
		cnt++;
	}

	if (colmissing[ART24_C_GEN1]) {
		updateValidation(val, ERROR, "Column [%s] missing",
				colnames[ART24_C_GEN1]);
		cnt++;
	}

	if (colmissing[ART24_C_GEN2]) {
		updateValidation(val, ERROR, "Column [%s] missing",
				colnames[ART24_C_GEN2]);
		cnt++;
	}

	if (colmissing[RES]) {
		updateValidation(val, ERROR, "One or more of the generational "
				"columns for [%s] are missing", colnames[RES]);
		cnt++;
	}

	/* after 10 missing columns an error message in included with a suggestion that the 
	   wrong cell was chosen */
	if (cnt > 10)
		updateValidation(val, ERROR, "\nMany columns missing, "
				"possible reasons:\n"
				"- incorrect cell chosen under the "
				"\"Data\" section\n"
				"- there is an empty column name in the data, "
				"the search for columns will end there");
}

/* 
 * validate the input given in the excel file. not all columns were added here
 * because not all of them will be used in the program, I chose the most
 * important ones to check, the others are the responsibility of the user
 */
void validateInput(DataColumn dc, const CurrentMember cm[static 1],
		const char key[static 1], const char input[static 1])
{
	const DataColumn floats[] = {
		SAL, PT, NORMRA, ART24_A_GEN1, ART24_A_GEN2, ART24_C_GEN1,
		ART24_C_GEN2, PREMIUM, CAP, CAPPS, RES, RESPS, CAPRED, TAUX
	};
	const DataColumn dates[] = {DOB, DOS, DOA, DOR};

	size_t lenf = sizeof(floats)/sizeof(floats[0]);
	size_t lend = sizeof(dates)/sizeof(dates[0]);
	struct date *tmp = 0;
	unsigned update = 0;
	const unsigned nofloat = 0x1;
	const unsigned neg = 0x2;
	const unsigned invaliddate = 0x4;
	const unsigned invalidstatus = 0x8;
	const unsigned invalidsex = 0x10;
	const unsigned invalidPT = 0x20;

	for (size_t i = 0; i < lenf; i++) {
		if (floats[i] == dc) {
			if (!isfloat(input))
				update += nofloat;
			if (input[0] == '-')
				update += neg;
			break;
		}
	}

	for (size_t i = 0; i < lend; i++) {
		if (dates[i] == dc) {
			tmp = newDate((unsigned)atoi(input), 0, 0, 0);
			if (0 == tmp) {
				update += invaliddate;
			} else {
				free(tmp);
			}
			break;
		}
	}

	if (STATUS == dc) {
		if (0 != strcmp(input, "ACT") && 0 != strcmp(input, "DEF"))
			update += invalidstatus;
	} else if (SEX == dc) {
		if (atoi(input) != 1 && atoi(input) != 2)
			update += invalidsex;
	} else if (PT == dc) {
		if (atof(input) > 1 || atof(input) < 0)
			update += invalidPT;
	}

	if (nofloat & update)
		updateValidation(val, ERROR, "Member [%d][%s] has [%s = %s], "
				"expected of the form %s", cm->id, cm->key,
				key, input, validMsg[FLOATERR]);
	if (neg & update)
		updateValidation(val, WARNING, "Member [%d][%s] has a negative"
				" %s [%s]", cm->id, cm->key, key, input);
	if (invaliddate & update)
		updateValidation(val, ERROR, "Member [%d][%s] has invalid date"
				" [%s = %s]", cm->id, cm->key, key, input); 

	if (invalidstatus & update)
		updateValidation(val, ERROR, "Member [%d][%s] has invalid "
				"status [%s = %s], expected ACT or DEF", 
				cm->id, cm->key, key, input); 
	if (invalidsex & update)
		updateValidation(val, ERROR, "Member [%d][%s] has invalid "
				"gender [%s = %s], expected 1(male) or "
				"2(female)", cm->id, cm->key, key, input); 
	if (invalidPT & update)
		updateValidation(val, WARNING, "Member [%d][%s] has [%s = %s],"
				"expected between 0 and 1", cm->id, cm->key,
				key, input); 
}
