#include "calculator.h"
#include "DCProgram.h"
#include "xlsxwriter.h"
#include "printresults.h"

static struct variable tc_print[TC_AMOUNT] = {
	[TC_KEY] = {"KEY", 0, .v.s = ""},
	[TC_DOC] = {"DOC", 0, .v.s = ""},
	[TC_AGE] = {"Age", 1, .v.d = 0.0},
	[TC_SAL] = {"Salary", 1, .v.d = 0.0},
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

unsigned printtc(DataSet *ds, unsigned tc)
{
	char results[BUFSIZ];
	char temp[64];
	char *dirname = ds->xl->dirname;
	int row = 0;
	CurrentMember *cm = &ds->cm[tc];

	lxw_workbook *wb = 0;
	lxw_worksheet *ws = 0;
	lxw_datetime DOC = (lxw_datetime){0};
	lxw_format *DOCformat = 0;
	lxw_format *nformat = 0;

	snprintf(temp, sizeof(temp), "TestCase%d", tc + 1);
	snprintf(results, sizeof(results), "%s/%s.xlsx", dirname, temp);

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
		DOC.year = cm->DOC[row]->year;
		DOC.month = cm->DOC[row]->month;
		DOC.day = cm->DOC[row]->day;
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
	char *ass[ASSET_AMOUNT] = {
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
			snprintf(tmp, len, "DBO RET %s %s", m[i], ass[j]);
			tc_print[TC_DBO + gen].name = strdup(tmp);
			to_free[TC_DBO + gen] = 1;
			snprintf(tmp, len, "NC RET %s %s", m[i], ass[j]);
			tc_print[TC_NC + gen].name = strdup(tmp);
			to_free[TC_NC + gen] = 1;
		}
	}

	for (unsigned i = 0; i < METHOD_AMOUNT - 1; i++) {
		for (unsigned j = 0; j < ASSET_AMOUNT; j++) {
			gen = j + ASSET_AMOUNT * i;
			snprintf(tmp, len, "PBO NC CF %s %s", m[i], ass[j]);
			tc_print[TC_PBONCCF + gen].name = strdup(tmp);
			to_free[TC_PBONCCF + gen] = 1;
			for (unsigned k = 0; k < CF_AMOUNT; k++) {
				gen = j + ASSET_AMOUNT * i
				+ ASSET_AMOUNT
				* (METHOD_AMOUNT - 1)
				* (1 + k) - 1;
				snprintf(tmp, len, "EBP %s %s %s",
						cf[k], m[i], ass[j]);
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
	char *s = tc_print[TC_KEY].v.s;
	size_t len = sizeof(tc_print[TC_KEY].v.s);

	snprintf(s, len, "%s", cm->key);

	tc_print[TC_AGE].v.d = cm->age[row];
	tc_print[TC_SAL].v.d = cm->sal[row];
	tc_print[TC_CONTRA].v.d = gensum(cm->PREMIUM, ER, row);
	tc_print[TC_DTHRISK].v.d = cm->CAPDTHRiskPart[row];
	tc_print[TC_DTHRES].v.d = cm->CAPDTHRESPart[row];
	tc_print[TC_CONTRC].v.d = gensum(cm->PREMIUM, EE, row);

	for (unsigned i = 0; i < MAXGEN; i++) {
		for (unsigned j = 0; j < EREE_AMOUNT; j++) {
			gen = a * i + a * MAXGEN * j;
			tc_print[TC_CAP + gen].v.d = cm->CAP[j][i][row];
			tc_print[TC_PREM + gen].v.d = cm->PREMIUM[j][i][row];
			tc_print[TC_RESPS + gen].v.d =
				cm->RESPS[PUC][j][i][row];
			tc_print[TC_RES + gen].v.d = cm->RES[PUC][j][i][row];
			tc_print[TC_CAP + gen].is_number = 1;
			tc_print[TC_PREM + gen].is_number = 1;
			tc_print[TC_RESPS + gen].is_number = 1;
			tc_print[TC_RES + gen].is_number = 1;
		}
	}
	tc_print[TC_TOTRESA].v.d = gensum(cm->RES[PUC], ER, row)
		+ gensum(cm->RESPS[PUC], ER, row);
	tc_print[TC_TOTRESC].v.d = gensum(cm->RES[PUC], EE, row)
		+ gensum(cm->RESPS[PUC], EE, row);

	tc_print[TC_REDCAPPUC].v.d = gensum(cm->REDCAP[PUC], ER, row)
		+ gensum(cm->REDCAP[PUC], EE, row);
	tc_print[TC_REDCAPTUC].v.d = gensum(cm->REDCAP[TUC], ER, row)
		+ gensum(cm->REDCAP[TUC], EE, row);
	tc_print[TC_REDCAPTUCPS1].v.d = gensum(cm->REDCAP[TUCPS_1], ER, row)
		+ gensum(cm->REDCAP[TUCPS_1], EE, row);

	tc_print[TC_RESPUC].v.d = gensum(cm->RES[PUC], ER, row)
		+ gensum(cm->RES[PUC], EE, row)
		+ gensum(cm->RESPS[PUC], ER, row)
		+ gensum(cm->RESPS[PUC], EE, row);
	tc_print[TC_RESTUC].v.d = gensum(cm->RES[TUC], ER, row)
		+ gensum(cm->RES[TUC], EE, row)
		+ gensum(cm->RESPS[TUC], ER, row)
		+ gensum(cm->RESPS[TUC], EE, row);
	tc_print[TC_RESTUCPS1].v.d = gensum(cm->RES[TUCPS_1], ER, row)
		+ gensum(cm->RES[TUCPS_1], EE, row)
		+ gensum(cm->RESPS[TUCPS_1], ER, row)
		+ gensum(cm->RESPS[TUCPS_1], EE, row);

	for (unsigned j = 0; j < METHOD_AMOUNT; j++) {
		for (unsigned i = 0; i < ART24GEN_AMOUNT; i++) {
			for (unsigned k = 0; k < EREE_AMOUNT; k++) {
				gen = i+ART24GEN_AMOUNT*(METHOD_AMOUNT*k + j);
				tc_print[TC_ART24 + gen].v.d =
					cm->ART24[j][k][i][row];
				tc_print[TC_ART24 + gen].is_number = 1;
			}
		}
	}

	tc_print[TC_FF].v.d = cm->FF[row + 1];
	tc_print[TC_QX].v.d = cm->qx[row + 1];
	tc_print[TC_WXDEF].v.d = cm->wxdef[row + 1];
	tc_print[TC_WXIMM].v.d = cm->wximm[row + 1];
	tc_print[TC_RETX].v.d = cm->retx[row + 1];
	tc_print[TC_KPX].v.d = cm->kPx[row + 1];
	tc_print[TC_NPK].v.d = cm->nPk[row + 1];
	tc_print[TC_VK].v.d = cm->vk[row + 1];
	tc_print[TC_VN].v.d = cm->vn[row + 1];

	a = 2;
	for (unsigned i = 0; i < METHOD_AMOUNT - 1; i++) {
		for (unsigned j = 0; j < ASSET_AMOUNT; j++) {
			gen = a * j + a * ASSET_AMOUNT * i;
			tc_print[TC_DBO + gen].v.d = cm->DBORET[i][j][row + 1];
			tc_print[TC_NC + gen].v.d = cm->NCRET[i][j][row + 1];
			tc_print[TC_DBO + gen].is_number = 1;
			tc_print[TC_NC + gen].is_number = 1;
		}
	}

	tc_print[TC_ASSETS115].v.d = cm->assets[PAR115][row + 1];
	tc_print[TC_ASSETS113].v.d = cm->assets[PAR113][row + 1];
	tc_print[TC_DBODTHRISK].v.d = cm->DBODTHRiskPart[row + 1];
	tc_print[TC_DBODTHRES].v.d = cm->DBODTHRESPart[row + 1];
	tc_print[TC_NCDTHRISK].v.d = cm->NCDTHRiskPart[row + 1];
	tc_print[TC_NCDTHRES].v.d = cm->NCDTHRESPart[row + 1];

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

static void free_print_names(void)
{
	for (unsigned i = 0; i < TC_AMOUNT; i++) {
		if (to_free[i]) free(tc_print[i].name);
	}
}
