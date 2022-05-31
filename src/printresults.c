#include "DCProgram.h"
#include "xlsxwriter.h"
#include "printresults.h"
#include "assumptions.h"
#include "common.h"

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
	buf_free(buf); \
	column++;
#define PRINT_LS(title, d) \
	PRINT_NUMBER(title, lump_sums.d)
#define PRINT_RES(title, d) \
	PRINT_NUMBER(title, reserves.d)

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
		case GENS_RES:
			PRINT_RES("Reserves PUC A", res[ER].puc);
			PRINT_RES("Reserves PUC C", res[EE].puc);
			PRINT_RES("Reserves TUC A", res[ER].tuc);
			PRINT_RES("Reserves TUC C", res[EE].tuc);
			PRINT_RES("Reserves TUC+1 A", res[ER].tucps_1);
			PRINT_RES("Reserves TUC+1 C", res[EE].tucps_1);
			PRINT_RES("Reserves PS A", ps[ER]);
			PRINT_RES("Reserves PS C", ps[EE]);
			break;
		case GENS_LUMP_SUM:
			PRINT_LS("Lump Sum A", lump_sum[ER]);
			PRINT_LS("Lump Sum C", lump_sum[EE]);
			PRINT_LS("Lump Sum PS A", ps[ER]);
			PRINT_LS("Lump Sum PS C", ps[EE]);
			PRINT_LS("Death Lump Sum A", death_lump_sum[ER]);
			PRINT_LS("Death Lump Sum C", death_lump_sum[EE]);
			PRINT_LS("Reduced Lump Sum PUC A", reduced[ER].puc);
			PRINT_LS("Reduced Lump Sum PUC C", reduced[EE].puc);
			PRINT_LS("Reduced Lump Sum TUC A", reduced[ER].tuc);
			PRINT_LS("Reduced Lump Sum TUC C", reduced[EE].tuc);
			PRINT_LS("Reduced Lump Sum TUC+1 A",
					reduced[ER].tucps_1);
			PRINT_LS("Reduced Lump Sum TUC+1 C",
					reduced[EE].tucps_1);
			break;
		default:
			assert(0);
			break;
	}
}

#undef PRINT_RES
#undef PRINT_LS
#undef PRINT_NUMBER

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
	print_gen_kind(wb, ws, proj, gen, GENS_RES);
	print_gen_kind(wb, ws, proj, gen, GENS_LUMP_SUM);
}

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
	buf_printf(buf, "%s Gen %d", title, gen + 1); \
	for (unsigned row = 0; row < MAXPROJ; row++) { \
		print_number(wb, ws, row+1, buf, proj[row].art24[gen].d); \
	} \
	buf_free(buf); \
	column++;

static void print_art24(lxw_workbook *wb, lxw_worksheet *ws,
		const struct projection *proj, size_t gen)
{
	assert(wb);
	assert(ws);
	assert(proj);
	assert(gen < ART24GEN_AMOUNT);

	char *buf = 0;

	PRINT_NUMBER("Art 24 PUC A", res[ER].puc);
	PRINT_NUMBER("Art 24 PUC C", res[EE].puc);
	PRINT_NUMBER("Art 24 TUC A", res[ER].tuc);
	PRINT_NUMBER("Art 24 TUC C", res[EE].tuc);
	PRINT_NUMBER("Art 24 TUC+1 A", res[ER].tucps_1);
	PRINT_NUMBER("Art 24 TUC+1 C", res[EE].tucps_1);
}

#undef PRINT_NUMBER

static void print_art24s(lxw_workbook *wb, lxw_worksheet *ws,
		const struct projection *proj)
{
	assert(wb);
	assert(ws);
	assert(proj);

	for (size_t i = 0; i < ART24GEN_AMOUNT; i++) {
		print_art24(wb, ws, proj, i);
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
#define PRINT_FACTOR(title, d) \
	PRINT_NUMBER(title, factor.d)
#define PRINT_RET(title, d, a) \
	PRINT_NUMBER(title, d[a])
#define PRINT_RETIREMENT(t1, t2) \
	PRINT_RET(t1" RET PUC PAR115", t2.puc, PAR115); \
	PRINT_RET(t1" RET TUC PAR115", t2.tuc, PAR115); \
	PRINT_RET(t1" RET PUC PAR113", t2.puc, PAR113); \
	PRINT_RET(t1" RET TUC PAR113", t2.tuc, PAR113); \
	PRINT_RET(t1" RET PUC MATH RES", t2.puc, MATHRES); \
	PRINT_RET(t1" RET TUC MATH RES", t2.tuc, MATHRES);
#define PRINT_DEATH(title, d) \
	PRINT_NUMBER(title" Death Reserves", d.death_res); \
	PRINT_NUMBER(title" Death Risk", d.death_risk); \
	PRINT_NUMBER(title" IC Death Reserves", d.ic_death_res); \
	PRINT_NUMBER(title" IC Death Risk", d.ic_death_risk);


static void print_factor(lxw_workbook *wb, lxw_worksheet *ws,
		const struct projection *proj)
{
	assert(wb);
	assert(ws);
	assert(proj);
	PRINT_FACTOR("FF", ff);
	PRINT_FACTOR("FF Service Cost", ff_sc);
	PRINT_FACTOR("Qx", qx);
	PRINT_FACTOR("turnover deferred", wxdef);
	PRINT_FACTOR("turnover immediate", wximm);
	PRINT_FACTOR("retirement probability", retx);
	PRINT_FACTOR("nPk", nPk);
	PRINT_FACTOR("kPx", kPx);
	PRINT_FACTOR("(1+DR)^-k", vk);
	PRINT_FACTOR("(1+DR)^-n", vn);
	PRINT_FACTOR("(1+DR113)^-k", vk113);
	PRINT_FACTOR("(1+DR113)^-n", vn113);
}

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
		case PROJ_PBO_NC_DEATH:
			PRINT_NUMBER("PBO NC Death", pbo_nc_death);
			break;
		case PROJ_DELTA_CAP_A:
			PRINT_NUMBER("Delta Cap A", delta_cap[ER]);
			break;
		case PROJ_DELTA_CAP_C:
			PRINT_NUMBER("Delta Cap C", delta_cap[EE]);
			break;
		case PROJ_EBP_DEATH_PBO:
			PRINT_NUMBER("PBO EBP Death", ebp_death[PBO]);
			break;
		case PROJ_EBP_DEATH_TBO:
			PRINT_NUMBER("TBO EBP Death", ebp_death[TBO]);
			break;
		case PROJ_DOC:
			PRINT_DATE("DoC", DOC);
			break;
		case PROJ_GENS:
			print_gens(wb, ws, proj);
			break;
		case PROJ_ART24:
			print_art24s(wb, ws, proj);
			break;
		case PROJ_FACTOR:
			print_factor(wb, ws, proj);
			break;
		case PROJ_DBO_RET:
			PRINT_RETIREMENT("DBO", dbo_ret);
			break;
		case PROJ_NC_RET:
			PRINT_RETIREMENT("NC", nc_ret);
			break;
		case PROJ_IC_NC_RET:
			PRINT_RETIREMENT("IC NC", ic_nc_ret);
			break;
		case PROJ_EBP_RET_PBO:
			PRINT_RETIREMENT("PBO EBP Retirement", ebp_ret[PBO]);
			break;
		case PROJ_EBP_RET_TBO:
			PRINT_RETIREMENT("TBO EBP Retirement", ebp_ret[TBO]);
			break;
		case PROJ_PBO_NC_RET:
			PRINT_RETIREMENT("PBO NC Retirement", pbo_nc_ret);
			break;
		case PROJ_DBO_DEATH:
			PRINT_DEATH("DBO", dbo_death);
			break;
		case PROJ_NC_DEATH:
			PRINT_DEATH("NC", nc_death);
			break;
		case PROJ_ASSETS:
			PRINT_NUMBER("Assets PAR115", assets.par115);
			PRINT_NUMBER("Assets PAR113", assets.par113);
			PRINT_NUMBER("Assets Math reserves", assets.math_res);
			break;
		default:
			assert(0);
			break;
	}
}

#undef PRINT_NUMBER
#undef PRINT_DATE
#undef PRINT_FACTOR
#undef PRINT_RET
#undef PRINT_RETIREMENT
#undef PRINT_DEATH

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
	print_proj_kind(wb, ws, proj, PROJ_ART24);
	print_proj_kind(wb, ws, proj, PROJ_FACTOR);
	print_proj_kind(wb, ws, proj, PROJ_DBO_RET);
	print_proj_kind(wb, ws, proj, PROJ_NC_RET);
	print_proj_kind(wb, ws, proj, PROJ_IC_NC_RET);
	print_proj_kind(wb, ws, proj, PROJ_DBO_DEATH);
	print_proj_kind(wb, ws, proj, PROJ_ASSETS);
	print_proj_kind(wb, ws, proj, PROJ_PBO_NC_RET);
	print_proj_kind(wb, ws, proj, PROJ_PBO_NC_DEATH);
	print_proj_kind(wb, ws, proj, PROJ_EBP_RET_PBO);
	print_proj_kind(wb, ws, proj, PROJ_EBP_RET_TBO);
	print_proj_kind(wb, ws, proj, PROJ_EBP_DEATH_PBO);
	print_proj_kind(wb, ws, proj, PROJ_EBP_DEATH_TBO);
}

static void print_status(lxw_worksheet *ws, unsigned status)
{
	if (status & ACT) {
		print_str(ws, "STATUS", "ACT");
	} else {
		print_str(ws, "STATUS", "DEF");
	}
}

#define PRINT_NUMBER(title, EREE, dc) \
	for (unsigned row = 0; row < MAXPROJ; row++) { \
		double amount = gen_sum(cm->proj[row].gens, EREE, dc, PUC); \
		print_number(wb, ws, row+1, title, amount); \
	} \
	column++;

static void print_test_case_results(lxw_workbook *wb, lxw_worksheet *ws,
		const CurrentMember *cm)
{
	assert(ws);
	assert(cm);

	print_str(ws, "KEY", cm->key);
	print_str(ws, "REGLEMENT", cm->regl);
	print_str(ws, "NAME", cm->name);
	print_status(ws, cm->status);
	print_dates(wb, ws, "DOB", cm->DOB);
	PRINT_NUMBER("Contr A", ER, PREMIUM);
	PRINT_NUMBER("Contr C", EE, PREMIUM);
	print_proj(wb, ws, cm->proj);
}

#undef PRINT_NUMBER

void print_test_case(const CurrentMember *cm)
{
	assert(cm);
	column = 0;
	char *results = 0;
	char *tmp = 0;

	lxw_workbook *wb = 0;
	lxw_worksheet *ws = 0;

	buf_printf(tmp, "TestCase");
	buf_printf(results, "%s.xlsx", tmp);

	if (strlen(results) > PATH_MAX) {
		die("file name [%s] too large.\n [%d] is maximum while file "
				"name is [%lu]",
				results, PATH_MAX, strlen(results));
	}

	wb = workbook_new(results);
	ws = workbook_add_worksheet(wb, tmp);
	worksheet_set_column(ws, 0, 512, 20, 0);
	buf_free(tmp);
	buf_free(results);
	print_test_case_results(wb, ws, cm);
	workbook_close(wb);
}

/*
 * ***************************************************************************
 * the section below is used to print all the results of the entire population
 * whereas above is used for a single test case
 * ***************************************************************************
 */

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

static void print_contents(lxw_worksheet *ws, Record *record, size_t num_member)
{
	Content *content = 0;
	for (size_t i = 0; i < record->num_titles; i++) {
		content = map_get_str(record->data, record->titles[i]);
		if (!content) {
			continue;
		}
		print_content(ws, content, num_member, i);
	}
}

static void print_record(lxw_worksheet *ws, Record *record, size_t num_member)
{
	assert(ws);
	assert(record);

	print_contents(ws, record, num_member);
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
	printf("Printing Data...\n");
	for (size_t col = 0; col < db->num_titles; col++) {
		worksheet_write_string(ws, 0, col, db->titles[col], 0);
		print_records(ws, db);
		column++;
	}
	printf("Printing Data complete.\n");
}

#define PRINT_RES(k, j, title) \
	for (size_t i = 0; i < buf_len(cm); i++) { \
		double amount = gen_sum(cm[i].proj[k].gens, j, RES, PUC) \
		+ gen_sum(cm[i].proj[k].gens, j, RESPS, PUC); \
		print_number(wb, ws, i + 1, title, amount); \
	} \
	column++;

#define PRINT_ART24(k, title) \
	for (size_t i = 0; i < buf_len(cm); i++) { \
		double amount = gen_sum_art24(cm[i].proj[k].art24, PUC); \
		print_number(wb, ws, i + 1, title, amount); \
	} \
	column++;

static void print_reserves(lxw_workbook *wb, lxw_worksheet *ws,
		const CurrentMember *cm)
{
	PRINT_RES(0, ER, "Res A 0");
	PRINT_RES(1, ER, "Res A 1");
	PRINT_RES(0, EE, "Res C 0");
	PRINT_RES(1, EE, "Res C 1");
	PRINT_ART24(0, "Art24 0");
	PRINT_ART24(1, "Art24 1");
}

#undef PRINT_RES
#undef PRINT_ART24

#define PRINT_RET(kind, m, ass, title) \
	for (size_t i = 0; i < buf_len(cm); i++) { \
		double amount = proj_sum(kind, cm[i].proj, m, ass); \
		print_number(wb, ws, i + 1, title, amount); \
	} \
	column++;

#define PRINT_RET_METHOD(m) \
	PRINT_RET(PROJ_DBO_RET, m, PAR115, "LIAB RET "#m" PAR115"); \
	PRINT_RET(PROJ_DBO_RET, m, PAR113, "LIAB RET "#m" PAR113"); \
	PRINT_RET(PROJ_DBO_RET, m, MATHRES, "LIAB RET "#m" MATH RES"); \
	PRINT_RET(PROJ_NC_RET, m, PAR115, "NC "#m);
	

static void print_retirement(lxw_workbook *wb, lxw_worksheet *ws,
		const CurrentMember *cm)
{
	PRINT_RET_METHOD(PUC);
	PRINT_RET_METHOD(TUC);
}

static void print_members(lxw_workbook *wb, lxw_worksheet *ws,
		const CurrentMember *cm)
{
	assert(ws);
	assert(cm);
	printf("Printing results...\n");
	print_reserves(wb, ws, cm);
	print_retirement(wb, ws, cm);
	printf("Printing results completed.\n");
}

void print_results(const Database *db, const CurrentMember *cm)
{
	column = 0;
	assert(db);
	assert(cm);
	char *results = 0;
	char *ws_name = 0;

	lxw_workbook *wb = 0;
	lxw_worksheet *ws = 0;

	buf_printf(ws_name, "data");
	buf_printf(results, "%s - results.xlsx", db->excel->name);

	if (strlen(results) > PATH_MAX) {
		die("file name [%s] too large.\n [%d] is maximum while file "
				"name is [%lu]",
				results, PATH_MAX, strlen(results));
	}

	wb = workbook_new(results);
	ws = workbook_add_worksheet(wb, ws_name);
	worksheet_set_column(ws, 0, 512, 20, 0);
	buf_free(ws_name);
	buf_free(results);

	print_database(ws, db);
	column++;
	print_members(wb, ws, cm);
	workbook_close(wb);
}
