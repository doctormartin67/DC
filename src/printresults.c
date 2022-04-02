#include "DCProgram.h"
#include "xlsxwriter.h"
#include "printresults.h"
#include "assumptions.h"
#include "common.h"

struct variable {
	char *name;
	unsigned is_number;
	Val v;
};

static struct variable tc_print[TC_AMOUNT] = {
	[TC_KEY] = {"KEY", 0, .v.s = ""},
	[TC_DOC] = {"DOC", 0, .v.s = ""},
	[TC_AGE] = {"Age", 1, .v.d = 0.0},
	[TC_SAL] = {"Salary", 1, .v.d = 0.0},
	[TC_NDOA] = {"Service DoA", 1, .v.d = 0.0},
	[TC_NDOE] = {"Service DoE", 1, .v.d = 0.0},
	[TC_CONTRA] = {"Contr A", 1, .v.d = 0.0},
	[TC_DTHRISK] = {"DTH Risk Part", 1, .v.d = 0.0},
	[TC_DTHRES] = {"DTH RES Part", 1, .v.d = 0.0},
	[TC_CONTRC] = {"Contr C", 1, .v.d = 0.0},

	[TC_TOTRESA] = {"Total Reserves A", 1, .v.d = 0.0},
	[TC_TOTRESC] = {"Total Reserves C", 1, .v.d = 0.0},

	[TC_REDCAPPUC] = {"RED CAP - PUC", 1, .v.d = 0.0},
	[TC_REDCAPTUC] = {"RED CAP - TUC", 1, .v.d = 0.0},
	[TC_REDCAPTUCPS1] = {"RED CAP - TUC PS+1", 1, .v.d = 0.0},

	[TC_RESPUC] = {"RES - PUC", 1, .v.d = 0.0},
	[TC_RESTUC] = {"RES - TUC", 1, .v.d = 0.0},
	[TC_RESTUCPS1] = {"RES - TUC PS+1", 1, .v.d = 0.0},

	[TC_FF] = {"FF", 1, .v.d = 0.0},
	[TC_QX] = {"qx", 1, .v.d = 0.0},
	[TC_WXDEF] = {"wx (Deferred)", 1, .v.d = 0.0},
	[TC_WXIMM] = {"wx (Immediate)", 1, .v.d = 0.0},
	[TC_RETX] = {"retx", 1, .v.d = 0.0},
	[TC_KPX] = {"kPx", 1, .v.d = 0.0},
	[TC_NPK] = {"nPk", 1, .v.d = 0.0},
	[TC_VK] = {"v^k", 1, .v.d = 0.0},
	[TC_VN] = {"v^n", 1, .v.d = 0.0},

	[TC_ASSETS115] = {"ASSETS PAR 115", 1, .v.d = 0.0},
	[TC_ASSETS113] = {"ASSETS PAR 113", 1, .v.d = 0.0},
	[TC_DBODTHRISK] = {"DBO DTH Risk Part", 1, .v.d = 0.0},
	[TC_DBODTHRES] = {"DBO DTH RES Part", 1, .v.d = 0.0},
	[TC_NCDTHRISK] = {"NC DTH Risk Part", 1, .v.d = 0.0},
	[TC_NCDTHRES] = {"NC DTH RES Part", 1, .v.d = 0.0},

	[TC_EBPDTHTBO] = {"EBP DTH TBO", 1, .v.d = 0.0},
	[TC_EBPDTHPBO] = {"EBP DTH PBO", 1, .v.d = 0.0},
	[TC_PBODTHNCCF] = {"PBO DTH NC CF", 1, .v.d = 0.0},
};

static unsigned to_free[TC_AMOUNT];

static void set_generational_column_names(void);
static void set_row_values(CurrentMember *cm, int row);
static void free_print_names(void);

static unsigned column;

static void print_str(lxw_worksheet *ws, const char *title, const char *str)
{
	assert(ws);
	assert(title);
	assert(str);

	worksheet_write_string(ws, 0, column, title, 0);
	for (unsigned row = 0; row < MAXPROJ; row++) {
		worksheet_write_string(ws, row+1, column, str, 0);
	}
	column++;
}

static void print_date(lxw_workbook *wb, lxw_worksheet *ws, unsigned row, 
		const char *title, const struct date *d)
{
	assert(wb);
	assert(ws);
	assert(title);
	assert(d);

	lxw_datetime dt = (lxw_datetime){0};
	lxw_format *dt_format = workbook_add_format(wb);
	format_set_num_format(dt_format, "dd/mm/yyyy");

	assert(row);
	if (1 != row) {
		worksheet_write_string(ws, 0, column, title, 0);
	}
	dt.year = d->year;
	dt.month = d->month;
	dt.day = d->day;
	dt.hour = dt.min = dt.sec = 0;
	worksheet_write_datetime(ws, row, column, &dt, dt_format);
}

static void print_dates(lxw_workbook *wb, lxw_worksheet *ws, const char *title,
	const struct date *d)
{
	assert(wb);
	assert(ws);
	assert(title);
	assert(d);
	for (unsigned row = 0; row < MAXPROJ; row++) {
		print_date(wb, ws, row+1, title, d);
	}
	column++;
}

static void print_number(lxw_workbook *wb, lxw_worksheet *ws, unsigned row,
		const char *title, double number)
{
	assert(wb);
	assert(ws);
	assert(title);

	lxw_format *nformat = 0;
	nformat = workbook_add_format(wb);
	format_set_num_format(nformat, "#,##0.00");

	assert(row);
	if (1 != row) {
		worksheet_write_string(ws, 0, column, title, 0);
	}
	worksheet_write_number(ws, row, column, number, nformat);
}
#define PRINT_NUMBER(title, d) \
	buf_printf(buf, "%s Gen %d", title, gen + 1); \
	for (unsigned row = 0; row < MAXPROJ; row++) { \
		print_number(wb, ws, row+1, buf, proj[row].gens[gen].d); \
	} \
	column++;

static void print_gen_kind(lxw_workbook *wb, lxw_worksheet *ws, 
		const struct projection *proj, size_t gen, GenerationKind kind)
{
	assert(wb);
	assert(ws);
	assert(proj);

	char *buf = 0;

	switch (kind) {
		case GENS_NONE:
			assert(0);
			break;
		case GENS_PREMIUM_A:
			PRINT_NUMBER("Premium A", premium[ER]);
			break;
		case GENS_PREMIUM_C:
			PRINT_NUMBER("Premium C", premium[EE]);
			break;
		case GENS_RP_A:
			PRINT_NUMBER("Risk Premium A", risk_premium[ER]);
			break;
		case GENS_RP_C:
			PRINT_NUMBER("Risk Premium C", risk_premium[EE]);
			break;
		default:
			assert(0);
			break;
	}
	assert(buf);
	buf_free(buf);
}

static void print_gen(lxw_workbook *wb, lxw_worksheet *ws,
		const struct projection *proj, size_t gen)
{
	assert(wb);
	assert(ws);
	assert(proj);
	print_gen_kind(wb, ws, proj, gen, GENS_PREMIUM_A);
	print_gen_kind(wb, ws, proj, gen, GENS_PREMIUM_C);
	print_gen_kind(wb, ws, proj, gen, GENS_RP_A);
	print_gen_kind(wb, ws, proj, gen, GENS_RP_C);
}

#undef PRINT_NUMBER

static void print_gens(lxw_workbook *wb, lxw_worksheet *ws,
		const struct projection *proj)
{
	assert(wb);
	assert(ws);
	assert(proj);

	for (size_t i = 0; i < MAXGEN; i++) {
		print_gen(wb, ws, proj, i);
	}
}

#define PRINT_NUMBER(title, d) \
	for (unsigned row = 0; row < MAXPROJ; row++) { \
		print_number(wb, ws, row+1, title, proj[row].d); \
	} \
	column++;
#define PRINT_DATE(title, d) \
	for (unsigned row = 0; row < MAXPROJ; row++) { \
		print_date(wb, ws, row+1, title, proj[row].d); \
	} \
	column++;

static void print_proj_kind(lxw_workbook *wb, lxw_worksheet *ws, 
		const struct projection *proj, ProjectionKind kind)
{
	assert(wb);
	assert(ws);
	assert(proj);
	switch (kind) {
		case PROJ_NONE:
			assert(0);
			break;
		case PROJ_AGE:
			PRINT_NUMBER("Age", age);
			break;
		case PROJ_NDOE:
			PRINT_NUMBER("Service DoE", nDOE);
			break;
		case PROJ_NDOA:
			PRINT_NUMBER("Service DoA", nDOA);
			break;
		case PROJ_SAL:
			PRINT_NUMBER("Salary", sal);
			break;
		case PROJ_AFSL:
			PRINT_NUMBER("AFSL", afsl);
			break;
		case PROJ_DEATH_RES:
			PRINT_NUMBER("Death RES", death_res);
			break;
		case PROJ_DEATH_RISK:
			PRINT_NUMBER("Death Risk", death_risk);
			break;
		case PROJ_DELTA_CAP_A:
			PRINT_NUMBER("Delta Cap A", delta_cap[ER]);
			break;
		case PROJ_DELTA_CAP_C:
			PRINT_NUMBER("Delta Cap C", delta_cap[EE]);
			break;
		case PROJ_DOC:
			PRINT_DATE("DoC", DOC);
			break;
		case PROJ_GENS:
			print_gens(wb, ws, proj);
			break;
		default:
			assert(0);
			break;
	}
}

#undef PRINT_NUMBER
#undef PRINT_DATE

static void print_proj(lxw_workbook *wb, lxw_worksheet *ws, 
		const struct projection *proj)
{
	assert(wb);
	assert(ws);
	assert(proj);

	print_proj_kind(wb, ws, proj, PROJ_DOC);
	print_proj_kind(wb, ws, proj, PROJ_AGE);
	print_proj_kind(wb, ws, proj, PROJ_NDOE);
	print_proj_kind(wb, ws, proj, PROJ_NDOA);
	print_proj_kind(wb, ws, proj, PROJ_SAL);
	print_proj_kind(wb, ws, proj, PROJ_AFSL);
	print_proj_kind(wb, ws, proj, PROJ_DEATH_RES);
	print_proj_kind(wb, ws, proj, PROJ_DEATH_RISK);
	print_proj_kind(wb, ws, proj, PROJ_DELTA_CAP_A);
	print_proj_kind(wb, ws, proj, PROJ_DELTA_CAP_C);
	print_proj_kind(wb, ws, proj, PROJ_GENS);
}

static void print_member(lxw_workbook *wb, lxw_worksheet *ws,
		const CurrentMember *cm)
{
	assert(ws);
	assert(cm);

	print_str(ws, "KEY", cm->key);
	print_dates(wb, ws, "DOB", cm->DOB);
	print_proj(wb, ws, cm->proj);
}

void print_test_case(const CurrentMember *cm, unsigned tc)
{
	char *results = 0;
	char *tmp = 0;

	lxw_workbook *wb = 0;
	lxw_worksheet *ws = 0;

	buf_printf(tmp, "TestCase%d", tc + 1);
	buf_printf(results, "%s.xlsx", tmp);

	if (strlen(results) > PATH_MAX) {
		die("file name [%s] too large.\n [%d] is maximum while file "
				"name is [%lu]",
				results, PATH_MAX, strlen(results));
	}

	wb = workbook_new(results);
	ws = workbook_add_worksheet(wb, tmp);
	worksheet_set_column(ws, 0, 200, 25, 0);
	buf_free(tmp);
	buf_free(results);
	print_member(wb, ws, cm);
	workbook_close(wb);
}

unsigned printtc(CurrentMember *cm, unsigned tc)
{
	char results[BUFSIZ];
	char temp[64];
	int row = 0;

	lxw_workbook *wb = 0;
	lxw_worksheet *ws = 0;
	lxw_datetime DOC = (lxw_datetime){0};
	lxw_format *DOCformat = 0;
	lxw_format *nformat = 0;

	snprintf(temp, sizeof(temp), "TestCase%d", tc + 1);
	snprintf(results, sizeof(results), "%s.xlsx", temp);

	if (strlen(results) > PATH_MAX) return NAME_PRERR;

	wb = workbook_new(results);
	ws = workbook_add_worksheet(wb, temp);
	DOCformat = workbook_add_format(wb);
	nformat = workbook_add_format(wb);
	format_set_num_format(DOCformat, "dd/mm/yyyy");
	format_set_num_format(nformat, "#,##0.00");
	worksheet_set_column(ws, 0, 200, 25, 0);

	printf("Printing Testcase...\n");

	set_generational_column_names();
	for (unsigned i = 0; i < TC_AMOUNT; i++) {
		if (0 == tc_print[i].name) continue;
		worksheet_write_string(ws, row, i, tc_print[i].name, 0);
	}

	while (row < MAXPROJ) {
		set_row_values(cm, row);
		DOC.year = cm->proj[row].DOC->year;
		DOC.month = cm->proj[row].DOC->month;
		DOC.day = cm->proj[row].DOC->day;
		DOC.hour = DOC.min = DOC.sec = 0;
		for (unsigned i = 0; i < TC_FF; i++) {
			if (0 == tc_print[i].name) continue;
			if (TC_DOC == i) {
				worksheet_write_datetime(ws, row + 1,
						i, &DOC, DOCformat);
				continue;	
			}
			if (tc_print[i].is_number)
				worksheet_write_number(ws, row + 1, i,
						tc_print[i].v.d, nformat);
			else
				worksheet_write_string(ws, row + 1, i,
						tc_print[i].v.s, 0);
		}

		if (row+1 < MAXPROJ) {
			for (unsigned i = TC_FF; i < TC_AMOUNT; i++) {
				if (0 == tc_print[i].name) continue;
				worksheet_write_number(ws, row + 2, i,
						tc_print[i].v.d, nformat);

			}
		}

		row++;
	}
	printf("Printing test case complete.\n");

	workbook_close(wb);
	free_print_names();
	return NO_PRERR;
}

/*
 * sets the names of the columns that don't all have an enum defined for them
 * because we use a for loop to loop over multiple columns, for example in the
 * case of generations
 */
static void set_generational_column_names(void)
{
	unsigned a = 4;
	unsigned gen = 0;
	size_t len = 0;
	char c = 0;
	char tmp[64];
	char *m[METHOD_AMOUNT] = {
		[PUC] = "PUC",
		[TUC] = "TUC",
		[TUCPS_1] = "TUC PS+1"
	};
	char *assets[ASSET_AMOUNT] = {
		[PAR115] = "PAR115",
		[MATHRES] = "RES",
		[PAR113] = "PAR113"
	};
	char *cf[CF_AMOUNT] = {
		[TBO] = "TBO",
		[PBO] = "PBO",
	};
	len = sizeof(tmp);

	for (unsigned i = 0; i < MAXGEN; i++) {
		for (unsigned j = 0; j < EREE_AMOUNT; j++) {
			c = (j == ER ? 'A' : 'C');
			gen = a * i + a * MAXGEN * j;
			snprintf(tmp, len, "CAP GEN %d %c", i + 1, c);
			tc_print[TC_CAP + gen].name = strdup(tmp);
			to_free[TC_CAP + gen] = 1;
			snprintf(tmp, len, "PREMIUM GEN %d %c", i + 1, c);
			tc_print[TC_PREM + gen].name = strdup(tmp);
			to_free[TC_PREM + gen] = 1;
			snprintf(tmp, len, "RESERVES PS GEN %d %c", i + 1, c);
			tc_print[TC_RESPS + gen].name = strdup(tmp);
			to_free[TC_RESPS + gen] = 1;
			snprintf(tmp, len, "RESERVES GEN %d %c", i + 1, c);
			tc_print[TC_RES + gen].name = strdup(tmp);
			to_free[TC_RES + gen] = 1;
		}
	}

	for (unsigned j = 0; j < METHOD_AMOUNT; j++) {
		for (unsigned i = 0; i < ART24GEN_AMOUNT; i++) {
			for (unsigned k = 0; k < EREE_AMOUNT; k++) {
				c = (k == ER ? 'A' : 'C');
				gen = i+ART24GEN_AMOUNT*(METHOD_AMOUNT*k + j);
				snprintf(tmp, len, "ART24 GEN %d %c %s",
						i + 1, c, m[j]);
				tc_print[TC_ART24 + gen].name = strdup(tmp);
				to_free[TC_ART24 + gen] = 1;
			}
		}
	}

	a = 2;
	for (unsigned i = 0; i < METHOD_AMOUNT - 1; i++) {
		for (unsigned j = 0; j < ASSET_AMOUNT; j++) {
			gen = a * j + a * ASSET_AMOUNT * i;
			snprintf(tmp, len, "DBO RET %s %s", m[i], assets[j]);
			tc_print[TC_DBO + gen].name = strdup(tmp);
			to_free[TC_DBO + gen] = 1;
			snprintf(tmp, len, "NC RET %s %s", m[i], assets[j]);
			tc_print[TC_NC + gen].name = strdup(tmp);
			to_free[TC_NC + gen] = 1;
		}
	}

	for (unsigned i = 0; i < METHOD_AMOUNT - 1; i++) {
		for (unsigned j = 0; j < ASSET_AMOUNT; j++) {
			gen = j + ASSET_AMOUNT * i;
			snprintf(tmp, len, "PBO NC CF %s %s", m[i], assets[j]);
			tc_print[TC_PBONCCF + gen].name = strdup(tmp);
			to_free[TC_PBONCCF + gen] = 1;
			for (unsigned k = 0; k < CF_AMOUNT; k++) {
				gen = j + ASSET_AMOUNT * i
					+ ASSET_AMOUNT
					* (METHOD_AMOUNT - 1)
					* (1 + k) - 1;
				snprintf(tmp, len, "EBP %s %s %s",
						cf[k], m[i], assets[j]);
				tc_print[TC_EBP + gen].name = strdup(tmp);
				to_free[TC_EBP + gen] = 1;
			}
		}
	}
}

/*
 * sets the values of all the columns for 1 row
 */
static void set_row_values(CurrentMember *cm, int row)
{
	unsigned a = 4;
	unsigned gen = 0;
	size_t size = strlen(tc_print[TC_KEY].v.s) + 1;
	char s[size];

	snprintf(s, size, "%s", cm->key);

	tc_print[TC_AGE].v.d = cm->proj[row].age;
	tc_print[TC_SAL].v.d = cm->proj[row].sal;
	tc_print[TC_NDOA].v.d = cm->proj[row].nDOA;
	tc_print[TC_NDOE].v.d = cm->proj[row].nDOE;
	tc_print[TC_CONTRA].v.d = gen_sum(cm->proj[row].gens, ER, PREMIUM,
			PUC);
	tc_print[TC_DTHRISK].v.d = cm->proj[row].death_risk;
	tc_print[TC_DTHRES].v.d = cm->proj[row].death_res;
	tc_print[TC_CONTRC].v.d = gen_sum(cm->proj[row].gens, EE, PREMIUM,
			PUC);

	for (unsigned i = 0; i < MAXGEN; i++) {
		for (unsigned j = 0; j < EREE_AMOUNT; j++) {
			gen = a * i + a * MAXGEN * j;
			tc_print[TC_CAP + gen].v.d =
				cm->proj[row].gens[i].lump_sums.lump_sum[j];
			tc_print[TC_PREM + gen].v.d =
				cm->proj[row].gens[i].premium[j];
			tc_print[TC_RESPS + gen].v.d =
				cm->proj[row].gens[i].reserves.ps[j];
			tc_print[TC_RES + gen].v.d =
				cm->proj[row].gens[i].reserves.res[j].puc;
			tc_print[TC_CAP + gen].is_number = 1;
			tc_print[TC_PREM + gen].is_number = 1;
			tc_print[TC_RESPS + gen].is_number = 1;
			tc_print[TC_RES + gen].is_number = 1;
		}
	}
	tc_print[TC_TOTRESA].v.d
		= gen_sum(cm->proj[row].gens, ER, RES, PUC)
		+ gen_sum(cm->proj[row].gens, ER, RESPS, 0);
	tc_print[TC_TOTRESC].v.d
		= gen_sum(cm->proj[row].gens, EE, RES, PUC)
		+ gen_sum(cm->proj[row].gens, EE, RESPS, 0);

	tc_print[TC_REDCAPPUC].v.d
		= gen_sum(cm->proj[row].gens, ER, CAPRED, PUC)
		+ gen_sum(cm->proj[row].gens, EE, CAPRED, PUC);
	tc_print[TC_REDCAPTUC].v.d
		= gen_sum(cm->proj[row].gens, ER, CAPRED, TUC)
		+ gen_sum(cm->proj[row].gens, EE, CAPRED, TUC);
	tc_print[TC_REDCAPTUCPS1].v.d
		= gen_sum(cm->proj[row].gens, ER, CAPRED, TUCPS_1)
		+ gen_sum(cm->proj[row].gens, EE, CAPRED, TUCPS_1);

	tc_print[TC_RESPUC].v.d
		= gen_sum(cm->proj[row].gens, ER, RES, PUC)
		+ gen_sum(cm->proj[row].gens, EE, RES, PUC)
		+ gen_sum(cm->proj[row].gens, ER, RESPS, 0)
		+ gen_sum(cm->proj[row].gens, EE, RESPS, 0);
	tc_print[TC_RESTUC].v.d
		= gen_sum(cm->proj[row].gens, ER, RES, TUC)
		+ gen_sum(cm->proj[row].gens, EE, RES, TUC)
		+ gen_sum(cm->proj[row].gens, ER, RESPS, 0)
		+ gen_sum(cm->proj[row].gens, EE, RESPS, 0);
	tc_print[TC_RESTUCPS1].v.d
		= gen_sum(cm->proj[row].gens, ER, RES, TUCPS_1)
		+ gen_sum(cm->proj[row].gens, EE, RES, TUCPS_1)
		+ gen_sum(cm->proj[row].gens, ER, RESPS, 0)
		+ gen_sum(cm->proj[row].gens, EE, RESPS, 0);

	for (unsigned i = 0; i < ART24GEN_AMOUNT; i++) {
		for (unsigned k = 0; k < EREE_AMOUNT; k++) {
			gen = i+ART24GEN_AMOUNT*(3*k + PUC);
			tc_print[TC_ART24 + gen].v.d =
				cm->proj[row].art24[i].res[k].puc;
			tc_print[TC_ART24 + gen].is_number = 1;
			gen = i+ART24GEN_AMOUNT*(3*k + TUC);
			tc_print[TC_ART24 + gen].v.d =
				cm->proj[row].art24[i].res[k].tuc;
			tc_print[TC_ART24 + gen].is_number = 1;
			gen = i+ART24GEN_AMOUNT*(3*k + TUCPS_1);
			tc_print[TC_ART24 + gen].v.d =
				cm->proj[row].art24[i].res[k].tucps_1;
			tc_print[TC_ART24 + gen].is_number = 1;
		}
	}

	if (row + 1 < MAXPROJ) {
		tc_print[TC_FF].v.d = cm->proj[row + 1].factor.ff;
		tc_print[TC_QX].v.d = cm->proj[row + 1].factor.qx;
		tc_print[TC_WXDEF].v.d = cm->proj[row + 1].factor.wxdef;
		tc_print[TC_WXIMM].v.d = cm->proj[row + 1].factor.wximm;
		tc_print[TC_RETX].v.d = cm->proj[row + 1].factor.retx;
		tc_print[TC_KPX].v.d = cm->proj[row + 1].factor.kPx;
		tc_print[TC_NPK].v.d = cm->proj[row + 1].factor.nPk;
		tc_print[TC_VK].v.d = cm->proj[row + 1].factor.vk;
		tc_print[TC_VN].v.d = cm->proj[row + 1].factor.vn;

		a = 2;

		tc_print[TC_ASSETS115].v.d = cm->proj[row + 1].assets.par115;
		tc_print[TC_ASSETS113].v.d = cm->proj[row + 1].assets.par113;
		tc_print[TC_DBODTHRISK].v.d = cm->proj[row + 1].dbo_death.death_risk;
		tc_print[TC_DBODTHRES].v.d = cm->proj[row + 1].dbo_death.death_res;
		tc_print[TC_NCDTHRISK].v.d = cm->proj[row + 1].nc_death.death_risk;
		tc_print[TC_NCDTHRES].v.d = cm->proj[row + 1].nc_death.death_res;

		for (unsigned i = 0; i < METHOD_AMOUNT - 1; i++) {
			for (unsigned j = 0; j < ASSET_AMOUNT; j++) {
				gen = j + ASSET_AMOUNT * i;
				tc_print[TC_PBONCCF + gen].v.d =
					cm->PBONCCF[i][j][row + 1];
				for (unsigned k = 0; k < CF_AMOUNT; k++) {
					gen = j + ASSET_AMOUNT * i
						+ ASSET_AMOUNT
						* (METHOD_AMOUNT - 1)
						* (1 + k) - 1;
					tc_print[TC_EBP + gen].v.d =
						cm->EBP[i][j][k][row + 1];
				}
			}
		}

		tc_print[TC_EBPDTHTBO].v.d = cm->EBPDTH[TBO][row + 1];
		tc_print[TC_EBPDTHPBO].v.d = cm->EBPDTH[PBO][row + 1];
		tc_print[TC_PBODTHNCCF].v.d = cm->PBODTHNCCF[row + 1];
	}
}

static void free_print_names(void)
{
	for (unsigned i = 0; i < TC_AMOUNT; i++) {
		if (to_free[i]) free(tc_print[i].name);
	}
}

static void print_content(lxw_worksheet *ws, Content *content,
		size_t num_member, int col)
{
	assert(content);
	switch (content->kind) {
		case CONTENT_NONE:
			return;
		case CONTENT_BOOLEAN:
			worksheet_write_boolean(ws, num_member + 1, col,
					content->val.b, 0);
			break;	
		case CONTENT_INT:
			worksheet_write_number(ws, num_member + 1, col,
					content->val.i, 0);
			break;	
		case CONTENT_DOUBLE:
			worksheet_write_number(ws, num_member + 1, col,
					content->val.d, 0);
			break;	
		case CONTENT_STRING:
		case CONTENT_ERROR:
			worksheet_write_string(ws, num_member + 1, col,
					content->val.s, 0);
			break;	
	}
}

static void print_record(lxw_worksheet *ws, Record *record, size_t num_member)
{
	assert(ws);
	assert(record);

	Content *content = 0;
	for (size_t i = 0; i < record->num_titles; i++) {
		content = map_get_str(record->data, record->titles[i]);
		if (!content) {
			continue;
		}
		print_content(ws, content, num_member, i);
	}
}

static void print_records(lxw_worksheet *ws, const Database *db)
{
	assert(ws);
	assert(db);
	for (size_t i = 0; i < db->num_records; i++) {
		print_record(ws, db->records[i], i);
	}
}

static void print_database(lxw_worksheet *ws, const Database *db)
{
	assert(ws);
	assert(db);
	for (size_t col = 0; col < db->num_titles; col++) {
		worksheet_write_string(ws, 0, col, db->titles[col], 0);
		print_records(ws, db);
	}
}

unsigned printresults(const Database db[static 1], CurrentMember cm[static 1])
{
	assert(db);
	assert(cm);
	char results[BUFSIZ];
	unsigned row = 0;
	unsigned col = 0;  
	unsigned index = 0;
	double DBORETPUCPAR = 0.0;
	double DBORETPUCRES = 0.0;
	double DBORETTUCPAR = 0.0;
	double DBORETTUCRES = 0.0;

	double NCRETPUCPAR = 0.0;
	double NCRETPUCRES = 0.0;
	double NCRETTUCPAR = 0.0;
	double NCRETTUCRES = 0.0;

	double ICNCRETPUCPAR = 0.0;
	double ICNCRETPUCRES = 0.0;
	double ICNCRETTUCPAR = 0.0;
	double ICNCRETTUCRES = 0.0;

	double ExpERContr = 0.0;
	double ExpEEContr = 0.0;
	double er = 0.0;

	double DBODTHRESPART = 0.0;
	double DBODTHRiskPART = 0.0;
	double NCDTHRESPART = 0.0;
	double NCDTHRiskPART = 0.0;

	double assetsPAR115 = 0.0;
	double assetsPAR113 = 0.0;
	double fassets = 0.0;
	double assetsRES = 0.0;

	double ART24TOT = 0.0;

	double ICNCDTHRESPART = 0.0;
	double ICNCDTHRiskPART = 0.0;

	lxw_workbook *wb = 0;
	lxw_worksheet *ws = 0;

	snprintf(results, sizeof(results), "%s", "results.xlsx");

	if (strlen(results) > PATH_MAX) return NAME_PRERR;

	wb = workbook_new(results);
	ws = workbook_add_worksheet(wb, "dataTY");
	worksheet_set_column(ws, 0, 250, 20, 0);

	printf("Printing Data...\n");
	print_database(ws, db);
	printf("Printing Data complete.\n");
	printf("Printing results...\n");

	col = db->num_titles + 1;
	worksheet_write_string(ws, row, col+index++, "DR", 0);
	worksheet_write_string(ws, row, col+index++, "DC NC", 0);
	worksheet_write_string(ws, row, col+index++, "Method Standard", 0);
	worksheet_write_string(ws, row, col+index++, "Method DBO", 0);
	worksheet_write_string(ws, row, col+index++, "Method Assets", 0);
	worksheet_write_string(ws, row, col+index++, "Method Death", 0);
	worksheet_write_string(ws, row, col+index++, "Admin Cost", 0);
	worksheet_write_string(ws, row, col+index++, "Age", 0);
	worksheet_write_string(ws, row, col+index++, "Salary Scale", 0);

	index++;
	worksheet_write_string(ws, row, col+index++, "LIAB_RET_PUC_PAR", 0);
	worksheet_write_string(ws, row, col+index++, "LIAB_RET_PUC_RES", 0);
	worksheet_write_string(ws, row, col+index++, "LIAB_RET_TUC_PAR", 0);
	worksheet_write_string(ws, row, col+index++, "LIAB_RET_TUC_RES", 0);
	worksheet_write_string(ws, row, col+index++, "NC_RET_PUC_PAR", 0);
	worksheet_write_string(ws, row, col+index++, "NC_RET_PUC_RES", 0);
	worksheet_write_string(ws, row, col+index++, "NC_RET_TUC_PAR", 0);
	worksheet_write_string(ws, row, col+index++, "NC_RET_TUC_RES", 0);
	worksheet_write_string(ws, row, col+index++, "ExpERContr", 0);
	worksheet_write_string(ws, row, col+index++, "ExpEEContr", 0);
	worksheet_write_string(ws, row, col+index++, "LIAB_DTH_RESPART", 0);
	worksheet_write_string(ws, row, col+index++, "LIAB_DTH_RISKPART", 0);
	worksheet_write_string(ws, row, col+index++, "NC_DTH_RESPART", 0);
	worksheet_write_string(ws, row, col+index++, "NC_DTH_RISKPART", 0);
	index++;
	worksheet_write_string(ws, row, col+index++, "Assets_PAR115", 0);
	worksheet_write_string(ws, row, col+index++, "Assets_PAR113", 0);
	worksheet_write_string(ws, row, col+index++, "Assets_RES", 0);
	worksheet_write_string(ws, row, col+index++, "DBO_PUC_PAR", 0);
	worksheet_write_string(ws, row, col+index++, "SC_ER_PUC_PAR", 0);
	worksheet_write_string(ws, row, col+index++, "DBO_TUC_PAR", 0);
	worksheet_write_string(ws, row, col+index++, "SC_ER_TUC_PAR", 0);
	worksheet_write_string(ws, row, col+index++, "DBO_PUC_RES", 0);
	worksheet_write_string(ws, row, col+index++, "SC_ER_PUC_RES", 0);
	worksheet_write_string(ws, row, col+index++, "DBO_TUC_RES", 0);
	worksheet_write_string(ws, row, col+index++, "SC_ER_TUC_RES", 0);

	while (row < db->num_records) {
		index = 0;
		worksheet_write_number(ws, row+1, col+index++, ass.DR, 0);
		worksheet_write_number(ws, row+1, col+index++, ass.DR, 0);
		worksheet_write_string(ws, row+1, col+index++, 
				(ass.method & mIAS ? "IAS" : "FAS"), 0);
		worksheet_write_string(ws, row+1, col+index++, 
				(ass.method & mTUC ? "TUC" : "PUC"), 0);
		worksheet_write_string(ws, row+1, col+index++, 
				(ass.method & mRES ? "RES" : 
				 (ass.method & mPAR115 ? "PAR115" :
				  "PAR113")), 0);
		worksheet_write_number(ws, row+1, col+index++, 
				(ass.method & mDTH ? 1 : 0), 0);
		worksheet_write_number(ws, row+1, col+index++, tff.admincost,
				0);
		worksheet_write_number(ws, row+1, col+index++, cm->proj[0].age, 0);
		worksheet_write_number(ws, row+1, col+index++,
				salaryscale(cm, 1), 0);
		index++;

		ExpERContr = MAX(0.0, MIN(1.0, NRA(cm, 1) - cm->proj[1].age))
			* gen_sum(cm->proj[1].gens, ER, PREMIUM, PUC);
		ExpEEContr = MAX(0.0, MIN(1.0, NRA(cm, 1) - cm->proj[1].age))
			* gen_sum(cm->proj[1].gens, EE, PREMIUM, PUC);

		for (size_t i = 0; i < MAXPROJ; i++) {
			DBODTHRESPART += cm->proj[i].dbo_death.death_res;
			DBODTHRiskPART += cm->proj[i].dbo_death.death_risk;
			NCDTHRESPART += cm->proj[i].nc_death.death_res;
			NCDTHRiskPART += cm->proj[i].nc_death.death_risk;
			ICNCDTHRESPART += cm->proj[i].nc_death.ic_death_res;
			ICNCDTHRiskPART += cm->proj[i].nc_death.ic_death_risk;
			assetsPAR115 += cm->proj[i].assets.par115;
			assetsPAR113 += cm->proj[i].assets.par113;
			assetsRES += cm->proj[i].assets.math_res;
		}


		for (size_t i = 0; i < ART24GEN_AMOUNT; i++) {
			for (size_t j = 0; j < EREE_AMOUNT; j++) {
				ART24TOT += cm->proj[1].art24[i].res[j].puc;
			}
		}

		/* Liability */
		worksheet_write_number(ws, row+1, col+index++, DBORETPUCPAR,
				0);
		worksheet_write_number(ws, row+1, col+index++, DBORETPUCRES,
				0);
		worksheet_write_number(ws, row+1, col+index++, DBORETTUCPAR,
				0);
		worksheet_write_number(ws, row+1, col+index++, DBORETTUCRES,
				0);

		/* Normal Cost */
		worksheet_write_number(ws, row+1, col+index++, NCRETPUCPAR, 0);
		worksheet_write_number(ws, row+1, col+index++, NCRETPUCRES, 0);
		worksheet_write_number(ws, row+1, col+index++, NCRETTUCPAR, 0);
		worksheet_write_number(ws, row+1, col+index++, NCRETTUCRES, 0);

		/* Expected contributions */
		worksheet_write_number(ws, row+1, col+index++, ExpERContr, 0);
		worksheet_write_number(ws, row+1, col+index++, ExpEEContr, 0);

		/* Death */
		worksheet_write_number(ws, row+1, col+index++, DBODTHRESPART,
				0);
		worksheet_write_number(ws, row+1, col+index++, DBODTHRiskPART,
				0);
		worksheet_write_number(ws, row+1, col+index++, NCDTHRESPART,
				0);
		worksheet_write_number(ws, row+1, col+index++, NCDTHRiskPART,
				0);

		index++;
		/* Assets */
		worksheet_write_number(ws, row+1, col+index++, assetsPAR115,
				0);
		worksheet_write_number(ws, row+1, col+index++, assetsPAR113,
				0);
		worksheet_write_number(ws, row+1, col+index++, assetsRES, 0);

		/* DBO + SC */
		/* DBO RET PUC PAR */
		fassets = (ass.method & PAR113 ? assetsPAR113 : assetsPAR115);
		worksheet_write_number(ws, row+1, col+index++, 
				MAX2(DBORETPUCPAR, fassets), 0);
		/* SC ER PUC PAR */
		worksheet_write_number(ws, row+1, col+index++, 
				MAX2(0.0, NCRETPUCPAR + ICNCRETPUCPAR
					- ExpEEContr), 0);
		/* DBO RET TUC PAR */
		worksheet_write_number(ws, row+1, col+index++, 
				MAX2(DBORETTUCPAR, fassets), 0);
		/* SC ER TUC PAR */
		er = (ass.method & mmaxERContr ?
				ExpERContr * (1 - tff.admincost)
				/ (1 + ass.taxes) : 0.0);
		worksheet_write_number(ws, row+1, col+index++, 
				MAX2(er, NCRETTUCPAR + ICNCRETTUCPAR
					- ExpEEContr), 0);
		/* DBO RET PUC RES */

		if (!(ass.method & mDTH)) {
			DBODTHRESPART = 0.0;
			DBODTHRiskPART = 0.0;
			NCDTHRESPART = 0.0;
			NCDTHRiskPART = 0.0;
			ICNCDTHRESPART = 0.0;
			ICNCDTHRiskPART = 0.0;
		}
		worksheet_write_number(ws, row+1, col+index++, 
				MAX3(DBORETPUCRES + DBODTHRESPART, assetsRES, 
					ART24TOT) + DBODTHRiskPART, 0);
		/* SC ER PUC RES */
		worksheet_write_number(ws, row+1, col+index++, 
				MAX2(0.0, NCRETPUCRES + ICNCRETPUCRES
					+ NCDTHRESPART + ICNCDTHRESPART)
				- ExpEEContr + NCDTHRiskPART + ICNCDTHRiskPART
				, 0);
		/* DBO RET TUC RES */
		worksheet_write_number(ws, row+1, col+index++, 
				MAX3(DBORETTUCRES + DBODTHRESPART, assetsRES, 
					ART24TOT) + DBODTHRiskPART, 0);
		/* SC ER TUC RES */
		worksheet_write_number(ws, row+1, col+index++, 
				MAX2(er, NCRETTUCRES + ICNCRETTUCRES
					+ NCDTHRESPART + ICNCDTHRESPART - ExpEEContr)
				+ NCDTHRiskPART + ICNCDTHRiskPART, 0);
		row++;
		cm++;
	}
	printf("Printing results complete.\n");
	return workbook_close(wb);
}
