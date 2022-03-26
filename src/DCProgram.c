#define _GNU_SOURCE
#include <assert.h>
#include "common.h"
#include "helperfunctions.h"
#include "errorexit.h"
#include "assumptions.h"

const char *const colnames[KEYS_AMOUNT] = {
	[KEY] = "KEY", [NOREGLEMENT] = "NO REGLEMENT", [NAAM] = "NAAM",
	[CONTRACT] = "CONTRACT", [STATUS] = "STATUS",
	[ACTIVECONTRACT] = "ACTIVE CONTRACT", [SEX] = "SEX", [MS] = "MS",
	[DOB] = "DOB", [DOE] = "DOE", [DOL] = "DOL", [DOS] = "DOS",
	[DOA] = "DOA", [DOR] = "DOR", [CATEGORIE] = "CATEGORIE", [SAL] = "SAL",
	[PG] = "PG", [PT] = "PT", [NORMRA] = "NRA", [ENF] = "# ENF",
	[TARIEF] = "TARIEF", [KO] = "KO", [RENTINV] = "Rent INV",
	[CONTRINV] = "Contr INV", [ART24] = "ART24",
	[ART24_A_GEN1] = "ART24_A_GEN1", 
	[ART24_A_GEN2] = "ART24_A_GEN2",
	[ART24_C_GEN1] = "ART24_C_GEN1",
	[ART24_C_GEN2] = "ART24_C_GEN2",
	[PREMIUM] = "PREMIUM", [CAP] = "CAP",
	[CAPPS] = "CAPPS", [CAPDTH] = "CAPDTH", [RES] = "RES",
	[RESPS] = "RESPS", [CAPRED] = "CAPRED", [TAUX] = "TAUX",
	[DELTA_CAP_A_GEN1] = "DELTA_CAP_A_GEN1",
	[DELTA_CAP_C_GEN1] = "DELTA_CAP_C_GEN1", [X10] = "X/10",
};

const char *const inscomb[INSCOMB_AMOUNT] = {
	[UKMS] = "UKMS", [UKZT] = "UKZT", [UKMT] = "UKMT", [MIXED] = "MIXED"
};

static int colmissing[KEYS_AMOUNT];

static Validator *val;

static double get_gen_amount(const Database *db, size_t num_member,
		DataColumn dc, size_t EREE, size_t gen)
{
	double result = 0.0;
	char *buf = 0;
	switch (EREE) {
		case ER:
			buf_printf(buf, "%s%s%d", colnames[dc], "_A_GEN",
					gen + 1);
			break;
		case EE:
			buf_printf(buf, "%s%s%d", colnames[dc], "_C_GEN",
					gen + 1);
			break;
		default:
			assert(0);
			break;
	}
	result = record_double(db, num_member, buf);
	buf_free(buf);
	return result;
}

static void set_methods(const Database *db, size_t num_member,
		struct methods *method, DataColumn dc, size_t EREE, size_t gen)
{
	double d = get_gen_amount(db, num_member, dc, EREE, gen);
	method->puc = method->tuc = method->tucps_1 = d;
}

static void set_generations(const Database *db, size_t num_member,
		struct generations *gen)
{
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		for (size_t j = 0; j < MAXGEN; j++) {
			gen[j].lump_sums.lump_sum[i] =
				get_gen_amount(db, num_member, CAP, i, j);
			gen[j].lump_sums.ps[i] =
				get_gen_amount(db, num_member, CAPPS, i, j);
			set_methods(db, num_member,
					&gen[j].lump_sums.reduced[i], CAPRED,
					i, j);

			gen[j].premium[i] =
				get_gen_amount(db, num_member, PREMIUM, i, j);
			gen[j].death_lump_sum[i] =
				get_gen_amount(db, num_member, CAPDTH, i, j);

			set_methods(db, num_member,
					&gen[j].reserves.res[i], RES, i, j);
			gen[j].reserves.ps[i] =
				get_gen_amount(db, num_member, RESPS, i, j);
		}
	}
}

static void set_art24(const Database *db, size_t num_member, struct art24 *a24)
{
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		for (size_t j = 0; j < ART24GEN_AMOUNT; j++) {
			set_methods(db, num_member, &a24[j].res[i], ART24,
					i, j);
			a24[j].i[i] = art24_interest_rates[i][j];
		}
	}
}

static void set_projection(const Database *db, size_t num_member,
		struct projection *p)
{
	set_generations(db, num_member, p->gens);
	for (size_t i = 0; i < MAXPROJ; i++) {
		set_art24(db, num_member, p[i].art24);
	}
}

static void set_projections(const Database *db, size_t num_member,
		struct projection *p)
{
	set_projection(db, num_member, p);
}

static CurrentMember create_member(Database *db, size_t num_member)
{
	CurrentMember cm = (CurrentMember){0};

	cm.key = record_string(db, num_member, colnames[KEY]);
	cm.regl = record_string(db, num_member, colnames[NOREGLEMENT]);
	cm.name = record_string(db, num_member, colnames[NAAM]);
	cm.contract = record_string(db, num_member, colnames[CONTRACT]);
	cm.status = 0;
	if (!strcmp(record_string(db, num_member, colnames[STATUS]), "ACT")) {
		cm.status += ACT;
	}
	if (record_boolean(db, num_member, colnames[ACTIVECONTRACT])) {
		cm.status += ACTCON;
	}
	if (record_int(db, num_member, colnames[SEX]) == 1) {
		cm.status += MALE;
	}
	if (!strcmp(record_string(db, num_member, colnames[MS]), "M")) {
		cm.status += MARRIED;
	}
	cm.DOB = newDate(record_int(db, num_member, colnames[DOB]), 0, 0, 0);
	cm.DOE = newDate(record_int(db, num_member, colnames[DOE]), 0, 0, 0);
	cm.DOL = newDate(record_int(db, num_member, colnames[DOL]), 0, 0, 0);
	cm.DOS = newDate(record_int(db, num_member, colnames[DOS]), 0, 0, 0);
	cm.DOA = newDate(record_int(db, num_member, colnames[DOA]), 0, 0, 0);
	cm.DOR = newDate(record_int(db, num_member, colnames[DOR]), 0, 0, 0);

	cm.category = record_string(db, num_member, colnames[CATEGORIE]);
	cm.sal[0] = record_double(db, num_member, colnames[SAL]);
	cm.PG = record_double(db, num_member, colnames[PG]);
	cm.PT = record_double(db, num_member, colnames[PT]);
	cm.NRA = record_double(db, num_member, colnames[NORMRA]);
	cm.kids = record_int(db, num_member, colnames[ENF]);
	cm.tariff = 0;
	for (unsigned i = 0; i < INSCOMB_AMOUNT; i++) {
		const char *s = record_string(db, num_member, colnames[TARIEF]);
		if (!strcmp(s, inscomb[i])) {
			cm.tariff = i;
		}
	}
	cm.KO = record_double(db, num_member, colnames[KO]);
	cm.annINV = record_double(db, num_member, colnames[RENTINV]);
	cm.contrINV = record_double(db, num_member, colnames[CONTRINV]);

	// all variables that have generations, employer and employee
	set_projections(db, num_member, cm.proj);
	char *buf = 0;
	for (unsigned j = 0; j < MAXGEN; j++) {
		buf_printf(buf, "%s%s%d", colnames[TAUX], "_A_GEN", j + 1);
		cm.TAUX[ER][j] = record_double(db, num_member, buf);
		buf_free(buf);
		buf_printf(buf, "%s%s%d", colnames[TAUX], "_C_GEN", j + 1);
		cm.TAUX[EE][j] = record_double(db, num_member, buf);
		buf_free(buf);
	}

	//-  MISCELLANEOUS  -
	cm.DELTACAP[ER][0] = record_double(db, num_member,
			colnames[DELTA_CAP_A_GEN1]);
	cm.DELTACAP[EE][0] = record_double(db, num_member,
			colnames[DELTA_CAP_C_GEN1]);
	cm.X10 = record_double(db, num_member, colnames[X10]);
	if (cm.tariff == MIXED && cm.X10 == 0) {
		printf("Warning: X/10 equals zero for %s but he has a "
				"MIXED contract\n", cm.key);
		printf("X/10 will be taken as 1 by default.\n");
		cm.X10 = 1;
	}
	return cm;
}

CurrentMember *create_members(Database *db)
{
	CurrentMember *cm = 0;
	for (size_t i = 0; i < db->num_records; i++) {
		buf_push(cm, create_member(db, i));
	}
	return cm;
}

double get_method_amount(struct methods m, size_t method)
{
	double result = 0.0;
	switch (method) {
		case PUC:
			result = m.puc; 
			break;
		case TUC:
			result = m.tuc; 
			break;
		case TUCPS_1:
			result = m.tucps_1; 
			break;
		default:
			assert(0);
			break;
	}
	return result;
}

#define SUM(a) \
	switch (method) { \
		case PUC: \
			  sum += a.puc; \
		break; \
		case TUC: \
			  sum += a.tuc; \
		break; \
		case TUCPS_1: \
			      sum += a.tucps_1; \
		break; \
	}

static double gen_sum_type(const struct generations *g, size_t EREE,
		DataColumn dc, size_t method)
{
	double sum = 0.0;
	switch (dc) {
		case CAP:
			for (size_t i = 0; i < MAXGEN; i++) {
				sum += g[i].lump_sums.lump_sum[EREE];
			}
			break;
		case CAPPS:
			for (size_t i = 0; i < MAXGEN; i++) {
				sum += g[i].lump_sums.ps[EREE];
			}
			break;
		case CAPRED:
			for (size_t i = 0; i < MAXGEN; i++) {
				SUM(g[i].lump_sums.reduced[EREE]);
			}
			break;
		case PREMIUM:
			for (size_t i = 0; i < MAXGEN; i++) {
				sum += g[i].premium[EREE];
			}
			break;
		case RP:
			for (size_t i = 0; i < MAXGEN; i++) {
				sum += g[i].risk_premium[EREE];
			}
			break;
		case CAPDTH:
			for (size_t i = 0; i < MAXGEN; i++) {
				sum += g[i].death_lump_sum[EREE];
			}
			break;
		case RES:
			for (size_t i = 0; i < MAXGEN; i++) {
				SUM(g[i].reserves.res[EREE]);
			}
			break;
		case RESPS:
			for (size_t i = 0; i < MAXGEN; i++) {
				sum += g[i].reserves.ps[EREE];
			}
			break;
		default:
			assert(0);
			break;
	}
	return sum;
}

#undef SUM

double gen_sum(const struct generations *g, size_t EREE, DataColumn dc,
		size_t method)
{
	double sum = 0.0;
	switch (dc) {
		case CAP:
		case CAPPS:
		case CAPRED:
		case PREMIUM:
		case RP:
		case CAPDTH:
		case RES:
		case RESPS:
			sum = gen_sum_type(g, EREE, dc, method);
			break;
		default:
			assert(0);
			break;
	}
	return sum;
}

double gen_sum_art24(const struct art24 *a24, size_t method)
{
	assert(a24);
	double sum = 0.0;
	for (size_t i = 0; i < EREE_AMOUNT; i++) {
		for (size_t j = 0; j < ART24GEN_AMOUNT; j++) {
			sum += get_method_amount(a24[j].res[i], method);
		}
	}
	return sum;
}

/* 
 * All missing columns are set to a WARNING,
 * then the important ones are set to ERROR
 */
void validateColumns(void)
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
