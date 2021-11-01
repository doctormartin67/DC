#include "userinterface.h"
#include "libraryheader.h"
#include "errorexit.h"
#include "interpreter.h"
#include "treeerrors.h"

const char *const validMsg[ERR_AMOUNT] = {
	[DATEERR] = "[dd/mm/yyyy]", 
	[FLOATERR] = "^[+-]?[0-9]+\\.?[0-9]*$", 
	[AGECORRERR] = "^[+-]?[0-9][0-9]?$", 
	[CELLERR] = "^[A-Z][A-Z]?[A-Z]?[1-9][0-9]*$"
};

const char *const widgetname[WIDGET_AMOUNT] = {
	[SHEETNAME] = "sheetname", [KEYCELL] = "keycell", [W_DOC_LY] = "DOC",
	[DR] = "DR", [AGECORR] = "agecorr", [INFL] = "infl",
	[TRM_PERCDEF] = "TRM_PercDef", [DR113] = "DR113",
	[INTERPRETERTEXT] = "interpretertext", [STANDARD] = "standard",
	[W_ASSETS_LY] = "assets", [PUCTUC] = "PUCTUC",
	[MAXPUCTUC] = "maxPUCTUC", [MAXERCONTR] = "maxERContr",
	[EVALUATEDTH] = "evaluateDTH", [RUNCHOICE] = "runchoice",
	[TESTCASEBOX] = "testcasebox", [TESTCASE] = "testcase",
	[OPENDCFILE] = "openDCFile", [SAVEASDCFILE] = "saveasDCFile",
	[OPENEXCELFILE] = "openExcelFile", [WINDOW] = "window",
	[INTERPRETERWINDOW] = "interpreterwindow", [MSGERR] = "MsgErr",
	[FILENAME] = "filename", [STARTSTOP] = "startstop"
};

const struct user_input ui_interpreter_variables[UI_AMOUNT] = {
	[UI_SS] = {"Salary Scale", INTERPRETERTEXT},
	[UI_TURNOVER] = {"Turnover", INTERPRETERTEXT},
	[UI_RETX] = {"Retirement Probability", INTERPRETERTEXT},
	[UI_NRA] = {"Normal Retirement Age", INTERPRETERTEXT},
	[UI_ADMINCOST] = {"Administration Cost", INTERPRETERTEXT},
	[UI_COSTRES] = {"Cost on Reserves", INTERPRETERTEXT},
	[UI_COSTKO] = {"Cost on Lump Sum Life", INTERPRETERTEXT},
	[UI_WD] = {"Profit Sharing for Mortality", INTERPRETERTEXT},
	[UI_PREPOST] = {"Immediate or Due payments", INTERPRETERTEXT},
	[UI_TERM] = {"Payment Frequency", INTERPRETERTEXT},
	[UI_LTINS] = {"Life Tables Insurer", INTERPRETERTEXT},
	[UI_LTTERM] = {"Life Tables After Termination", INTERPRETERTEXT},
	[UI_CONTRA] = {"Employer Contribution", INTERPRETERTEXT},
	[UI_CONTRC] = {"Employee Contribution", INTERPRETERTEXT}
};

const struct user_input ui_fixed_variables[UI_FIXED_AMOUNT] = {
	[UI_SHEETNAME] = {"Sheet name", SHEETNAME},
	[UI_DOC] = {"DOC", W_DOC_LY},
	[UI_DR] = {"DR", DR},
	[UI_AGECORR] = {"Age Correction", AGECORR},
	[UI_INFL] = {"Inflation", INFL},
	[UI_TRM_PERCDEF] = {"Termination percentage", TRM_PERCDEF},
	[UI_DR113] = {"DR $113", DR113}
};

const struct user_input ui_method_variables[COMBO_AMOUNT] = {
	[COMBO_STANDARD] = {"Standard IAS/FAS", STANDARD},
	[COMBO_ASSETS] = {"Assets", W_ASSETS_LY},
	[COMBO_PUCTUC] = {"PUC/TUC", PUCTUC},
	[COMBO_MAXPUCTUC] = {"max(PUC, TUC)", MAXPUCTUC},
	[COMBO_MAXERCONTR] = {"max(NC, ER Contr)", MAXERCONTR},
	[COMBO_EVALDTH] = {"Evaluate Death", EVALUATEDTH}
};

GtkWidget *widgets[WIDGET_AMOUNT];
static GtkBuilder *builder;

enum {HT_USER_INPUT_SIZE = 131};
static Hashtable *ht_user_input_LY;
static Hashtable *ht_user_input_TY;

static GtkWidget *buildWidget(const char *);

void userinterface()
{
	gtk_init(0, 0);

	ht_user_input_LY = newHashtable(HT_USER_INPUT_SIZE, 0);
	ht_user_input_TY = newHashtable(HT_USER_INPUT_SIZE, 0);

	builder = gtk_builder_new_from_file(GLADEFILE);

	for (unsigned i = 0; i < WIDGET_AMOUNT; i++)
		widgets[i] = buildWidget(widgetname[i]);

	gtk_builder_connect_signals(builder, 0);
	g_signal_connect(widgets[WINDOW], "destroy",
			G_CALLBACK(gtk_main_quit), 0);

	gtk_widget_show(widgets[WINDOW]);

	gtk_main();
}

static GtkWidget *buildWidget(const char w[static 1])
{
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, w));
	if (0 == widget) die("incorrect builder name");
	return widget;
}

Hashtable *get_user_input(unsigned ui)
{
	if (USER_INPUT_LY == ui) {
		return ht_user_input_LY;
	} else if (USER_INPUT_TY == ui) { 
		return ht_user_input_TY;
	} else die("ui not equal to USER_INPUT_LY or USER_INPUT_TY");

	return 0;
}

void set_user_input(Hashtable ht[static 1])
{
	char tmp[64];
	char *kc = 0;
	const char *key = 0;
	unsigned wgt = 0;

	kc = strdup(gtk_entry_get_text(GTK_ENTRY(widgets[KEYCELL])));
	upper(kc);
	ht_set("keycell", kc, ht);
	free(kc);
	kc = 0;
	
	for (unsigned i = 0; i < UI_FIXED_AMOUNT; i++) {
		key = ui_fixed_variables[i].key;
		wgt = ui_fixed_variables[i].widget;
		ht_set(key, gtk_entry_get_text(GTK_ENTRY(widgets[wgt])), ht);
	}

	for (unsigned i = 0; i < METHOD_AMOUNT; i++) {
		key = ui_method_variables[i].key;
		wgt = ui_method_variables[i].widget;
		snprintf(tmp, sizeof(tmp), "%d", gtk_combo_box_get_active(
					GTK_COMBO_BOX(widgets[wgt])));
		ht_set(key, tmp, ht);
	}

	printHashtable(ht);
}

void update_user_interface(Hashtable ht[static 1])
{
	/* --- Data --- */
	char s[BUFSIZ];
	const char *key = 0;
	unsigned wgt = 0;

	snprintf(s, sizeof(s), "File set to run:\n%s", ht_get("fname", ht));
	gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), s); 

	for (unsigned i = 0; i < UI_FIXED_AMOUNT; i++) {
		key = ui_fixed_variables[i].key;
		wgt = ui_fixed_variables[i].widget;
		gtk_entry_set_text(GTK_ENTRY(widgets[wgt]), ht_get(key, ht));
	}

	for (unsigned i = 0; i < METHOD_AMOUNT; i++) {
		key = ui_method_variables[i].key;
		wgt = ui_method_variables[i].widget;
		gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[wgt]),
				atoi(ht_get(key, ht)));
	}
}

void validateUI(Validator val[static 1], Hashtable ht[static 1])
{
	unsigned err = 0;
	const char *key = 0;
	for (unsigned i = 0; i < UI_AMOUNT; i++) {
		key = ui_interpreter_variables[i].key;
		if (!ht_get(key, ht)) {
			updateValidation(val, ERROR, "%s undefined", key);
			err++;
		}
	}
	for (unsigned i = 0; i < UI_FIXED_AMOUNT; i++) {
		key = ui_fixed_variables[i].key;
		if (!ht_get(key, ht)) {
			updateValidation(val, ERROR, "%s undefined", key);
			err++;
		}
	}
	for (unsigned i = 0; i < COMBO_AMOUNT; i++) {
		key = ui_method_variables[i].key;
		if (!ht_get(key, ht)) {
			updateValidation(val, ERROR, "%s undefined", key);
			err++;
		}
	}
	if (err) return;

	key = ht_get(ui_fixed_variables[UI_DOC].key, ht);
	size_t len = 0;
	register unsigned colcnt = 0;
	char temp[strlen(key) + 1];
	const char *kc = ht_get("keycell", ht);
	const char *pt = 0;
	char *day = 0, *month = 0, *year = 0;
	const char *tc = 0;
	struct date *tempDate = 0;
	struct casetree *ct = 0;

	/* ----- Check File -----*/
	if (!ht_get("fname", ht))
		updateValidation(val, ERROR, "No file selected to run");

	/* ----- Check keycell -----*/
	len = strlen(kc);

	if (kc[0] < 'A' || kc[0] > 'Z') {
		updateValidation(val, ERROR, "KEY cell [%s], "
				"expected of the form %s", 
				ht_get("keycell", ht), validMsg[CELLERR]);
	} else if (len > 1) {
		pt = kc + 1;
		colcnt++;

		while (!isdigit(*pt)) {
			pt++;
			colcnt++;
		}

		if (colcnt > 3) {
			updateValidation(val, ERROR, "KEY cell [%s], "
					"expected of the form %s", 
					ht_get("keycell", ht),
					validMsg[CELLERR]);
		}

		if ('\0' == *pt) {
			updateValidation(val, ERROR, "KEY cell [%s], "
					"expected of the form %s", 
					ht_get("keycell", ht),
					validMsg[CELLERR]);
		}

		while (isdigit(*pt)) pt++;

		if ('\0' != *pt) {
			updateValidation(val, ERROR, "KEY cell [%s], "
					"expected of the form %s", 
					ht_get("keycell", ht),
					validMsg[CELLERR]);
		}
	} else if (1 == len)
		updateValidation(val, ERROR, "KEY cell [%s], "
				"expected of the form %s", 
				ht_get("keycell", ht),
				validMsg[CELLERR]);

	/* ----- Check DOC -----*/

	snprintf(temp, sizeof(temp), "%s", key);

	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	if (0 == day || 0 == month || 0 == year) {
		updateValidation(val, ERROR, "DOC [%s], "
				"expected of the form %s",
				key, validMsg[DATEERR]);
	} else {
		if (!isint(day) || !isint(month) || !isint(year)) {
			updateValidation(val, ERROR, "DOC [%s], expected of "
					"the form %s", 
					key, validMsg[DATEERR]);
		}

		tempDate = newDate(0, atoi(year), atoi(month), atoi(day));
		if (0 == tempDate) {
			updateValidation(val, ERROR, "DOC [%s], "
					"expected of the form %s", 
					key, validMsg[DATEERR]);
		}
		free(tempDate);
	}

	/* ----- Check DR -----*/
	key = ht_get(ui_fixed_variables[UI_DR].key, ht);
	if (!isfloat(key)) {
		updateValidation(val, ERROR, "DR [%s], "
				"expected of the form %s",
				key, validMsg[FLOATERR]);
	}

	/* ----- Check Age Correction -----*/
	key = ht_get(ui_fixed_variables[UI_AGECORR].key, ht);
	if (!isint(key)) {
		updateValidation(val, ERROR, "Age Correction [%s], "
				"expected of the form %s", 
				key, validMsg[AGECORRERR]);
	}

	/* ----- Check Inflation -----*/
	key = ht_get(ui_fixed_variables[UI_INFL].key, ht);
	if (!isfloat(key)) {
		updateValidation(val, ERROR, "Inflation [%s], "
				"expected of the form %s", 
				key, validMsg[FLOATERR]);
	}

	/* ----- Check Termination percentage -----*/
	key = ht_get(ui_fixed_variables[UI_TRM_PERCDEF].key, ht);
	if (!isfloat(key)) {
		updateValidation(val, ERROR, "Termination % [%s] (usually 1), "
				"expected of the form %s", 
				key, validMsg[FLOATERR]);
	}

	/* ----- Check DR 113 -----*/
	key = ht_get(ui_fixed_variables[UI_DR113].key, ht);
	if (!isfloat(key)) {
		updateValidation(val, WARNING, "DR $113 [%s], "
				"expected of the form %s", 
				key, validMsg[FLOATERR]);
	}

	init_var(0);
	/* ----- Check Tree Variables -----*/
	for (unsigned i = 0; i < UI_AMOUNT; i++) {
		ct = plantTree(strclean(ht_get(ui_interpreter_variables[i].key,
						ht)));
		if (NOERR != getterrno()) {
			updateValidation(val, ERROR, "%s: %s",
					ui_interpreter_variables[i].key,
					strterror(getterrno()));
			setterrno(NOERR);
		}

		chopTree(ct);
		ct = 0;
	}

	/* ----- Check test case -----*/
	tc = gtk_entry_get_text(GTK_ENTRY(widgets[TESTCASE]));
	if (!isint(tc) || atoi(tc) < 1) {
		updateValidation(val, ERROR, "test case [%s] is not valid",
				tc);
	}
}

/*
 * This is a helper function that will create a temporary DataSet to validate
 * the data, at which point it is instantly freed. The Validator val is
 * updated in the process
 */
void validateData(Validator val[static 1], Hashtable ht[static 1])
{
	DataSet *ds = 0;
	if (val->status == OK) {
		ds = createDS(val, ht); 
		freeDS(ds);
	}
}
