#include <assert.h>
#include "userinterface.h"
#include "helperfunctions.h"
#include "errorexit.h"

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

static const struct user_input ui_interpreter_variables[UI_AMOUNT] = {
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

static struct user_input ui_fixed_variables[UI_FIXED_AMOUNT] = {
	[UI_SHEETNAME] = {"Sheet name", SHEETNAME},
	[UI_DOC] = {"DOC", W_DOC_LY},
	[UI_DR] = {"DR", DR},
	[UI_AGECORR] = {"Age Correction", AGECORR},
	[UI_INFL] = {"Inflation", INFL},
	[UI_TRM_PERCDEF] = {"Termination percentage", TRM_PERCDEF},
	[UI_DR113] = {"DR $113", DR113}
};

static const struct user_input ui_method_variables[COMBO_AMOUNT] = {
	[COMBO_STANDARD] = {"Standard IAS/FAS", STANDARD},
	[COMBO_ASSETS] = {"Assets", W_ASSETS_LY},
	[COMBO_PUCTUC] = {"PUC/TUC", PUCTUC},
	[COMBO_MAXPUCTUC] = {"max(PUC, TUC)", MAXPUCTUC},
	[COMBO_MAXERCONTR] = {"max(NC, ER Contr)", MAXERCONTR},
	[COMBO_EVALDTH] = {"Evaluate Death", EVALUATEDTH}
};

static const struct user_input ui_special_variables[SPECIAL_AMOUNT] = {
	[SPECIAL_KEYCELL] = {"Key Cell", KEYCELL},
	[SPECIAL_FILENAME] = {"File Name", FILENAME}
};

GtkWidget *widgets[WIDGET_AMOUNT];
static GtkBuilder *builder;

static Map user_input;

static GtkWidget *buildWidget(const char *);

void userinterface()
{
	gtk_init(0, 0);

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

const char *get_ui_key(unsigned var, unsigned type)
{
	const char *s = 0;
	char *key = 0;
	switch (type) {
		case UI_INT:
			assert(var < UI_AMOUNT);
			s = ui_interpreter_variables[var].key;	
			break;
		case UI_FIXED:
			assert(var < UI_FIXED_AMOUNT);
			s = ui_fixed_variables[var].key;
			break;
		case UI_COMBO:
			assert(var < COMBO_AMOUNT);
			s = ui_method_variables[var].key;
			break;
		case UI_SPECIAL:
			assert(var < SPECIAL_AMOUNT);
			s = ui_special_variables[var].key;
			break;
		default :
			die("should never reach here");
	}

	assert(s);
	key = strdup(s);
	upper(key);
	return key;
}

unsigned get_ui_widget(unsigned var, unsigned type)
{
	unsigned wgt = 0;
	switch (type) {
		case UI_INT:
			assert(var < UI_AMOUNT);
			wgt = ui_interpreter_variables[var].widget;	
			break;
		case UI_FIXED:
			assert(var < UI_FIXED_AMOUNT);
			wgt = ui_fixed_variables[var].widget;
			break;
		case UI_COMBO:
			assert(var < COMBO_AMOUNT);
			wgt = ui_method_variables[var].widget;
			break;
		default :
			die("should never reach here");
	}

	return wgt;
}

Map *get_user_input(void)
{
	return &user_input;
}

void set_user_input(void)
{
	char *tmp = 0;
	char *kc = 0;
	const char *key = 0;
	unsigned wgt = 0;

	kc = strdup(gtk_entry_get_text(GTK_ENTRY(widgets[KEYCELL])));
	upper(kc);
	map_put_str(&user_input, get_ui_key(SPECIAL_KEYCELL, UI_SPECIAL),
	strdup(kc));
	free(kc);
	kc = 0;

	for (unsigned i = 0; i < UI_FIXED_AMOUNT; i++) {
		key = get_ui_key(i, UI_FIXED);
		wgt = get_ui_widget(i, UI_FIXED);
		map_put_str(&user_input, key,
				strdup(gtk_entry_get_text(GTK_ENTRY(widgets[wgt]))));
	}

	for (unsigned i = 0; i < COMBO_AMOUNT; i++) {
		key = get_ui_key(i, UI_COMBO);
		wgt = get_ui_widget(i, UI_COMBO);
		buf_printf(tmp, "%d", gtk_combo_box_get_active(
					GTK_COMBO_BOX(widgets[wgt])));
		map_put_str(&user_input, key, strdup(tmp));
		buf_free(tmp);
	}
}

void update_user_interface(void)
{
	/* --- Data --- */
	char *s = 0;
	char *key = 0;
	const char *value = 0;
	unsigned wgt = 0;

	key = strdup(get_ui_key(SPECIAL_FILENAME, UI_SPECIAL));
	upper(key);
	value = map_get_str(&user_input, key);
	if (value) {
		buf_printf(s, "File set to run:\n%s", value);
		gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), s); 
		buf_free(s);
	}

	for (unsigned i = 0; i < UI_FIXED_AMOUNT; i++) {
		key = strdup(get_ui_key(i, UI_FIXED));
		upper(key);
		value = map_get_str(&user_input, key);
		wgt = get_ui_widget(i, UI_FIXED);
		if (value)
			gtk_entry_set_text(GTK_ENTRY(widgets[wgt]), value);
	}

	for (unsigned i = 0; i < COMBO_AMOUNT; i++) {
		key = strdup(get_ui_key(i, UI_COMBO));
		upper(key);
		value = map_get_str(&user_input, key);
		wgt = get_ui_widget(i, UI_COMBO);
		if (value)
			gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[wgt]),
					atoi(value));
	}
}

void validateUI(Validator val[static 1])
{
	unsigned err = 0;
	const char *key = 0;
	const char *value = 0;
	for (unsigned i = 0; i < UI_AMOUNT; i++) {
		key = get_ui_key(i, UI_INT);
		if (!map_get_str(&user_input, key)) {
			updateValidation(val, ERROR, "%s undefined", key);
			err++;
		}
	}
	for (unsigned i = 0; i < UI_FIXED_AMOUNT; i++) {
		key = get_ui_key(i, UI_FIXED);
		if (!map_get_str(&user_input, key)) {
			updateValidation(val, ERROR, "%s undefined", key);
			err++;
		}
	}
	for (unsigned i = 0; i < COMBO_AMOUNT; i++) {
		key = get_ui_key(i, UI_COMBO);
		if (!map_get_str(&user_input, key)) {
			updateValidation(val, ERROR, "%s undefined", key);
			err++;
		}
	}
	if (err) return;

	key = get_ui_key(SPECIAL_KEYCELL, UI_SPECIAL);
	value = map_get_str(&user_input, key);
	size_t len = 0;
	register unsigned colcnt = 0;
	const char *kc = value;
	const char *pt = 0;
	char *day = 0, *month = 0, *year = 0;
	const char *tc = 0;
	struct date *tempDate = 0;

	/* ----- Check File -----*/
	if (!map_get_str(&user_input,
				get_ui_key(SPECIAL_FILENAME, UI_SPECIAL))) {
		updateValidation(val, ERROR, "No file selected to run");
	}

	/* ----- Check keycell -----*/
	len = strlen(kc);

	if (kc[0] < 'A' || kc[0] > 'Z') {
		updateValidation(val, ERROR, "KEY cell [%s], "
				"expected of the form %s", 
				kc, validMsg[CELLERR]);
	} else if (len > 1) {
		pt = kc + 1;
		colcnt++;

		while (!isdigit(*pt)) {
			pt++;
			colcnt++;
		}

		if (colcnt > 3) {
			updateValidation(val, ERROR, "KEY cell [%s], "
					"expected of the form %s", kc,
					validMsg[CELLERR]);
		}

		if ('\0' == *pt) {
			updateValidation(val, ERROR, "KEY cell [%s], "
					"expected of the form %s", kc,
					validMsg[CELLERR]);
		}

		while (isdigit(*pt)) pt++;

		if ('\0' != *pt) {
			updateValidation(val, ERROR, "KEY cell [%s], "
					"expected of the form %s", kc,
					validMsg[CELLERR]);
		}
	} else if (1 == len)
		updateValidation(val, ERROR, "KEY cell [%s], "
				"expected of the form %s", kc,
				validMsg[CELLERR]);

	/* ----- Check DOC -----*/
	key = get_ui_key(UI_DOC, UI_FIXED);
	value = map_get_str(&user_input, key);
	char temp[strlen(value) + 1];
	snprintf(temp, sizeof(temp), "%s", value);

	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	if (0 == day || 0 == month || 0 == year) {
		updateValidation(val, ERROR, "DOC [%s], "
				"expected of the form %s",
				value, validMsg[DATEERR]);
	} else {
		if (!isint(day) || !isint(month) || !isint(year)) {
			updateValidation(val, ERROR, "DOC [%s], expected of "
					"the form %s", 
					value, validMsg[DATEERR]);
		}

		tempDate = newDate(0, atoi(year), atoi(month), atoi(day));
		if (0 == tempDate) {
			updateValidation(val, ERROR, "DOC [%s], "
					"expected of the form %s", 
					value, validMsg[DATEERR]);
		}
	}

	/* ----- Check DR -----*/
	key = get_ui_key(UI_DR, UI_FIXED);
	value = map_get_str(&user_input, key);
	if (!isfloat(value)) {
		updateValidation(val, ERROR, "DR [%s], "
				"expected of the form %s",
				value, validMsg[FLOATERR]);
	}

	/* ----- Check Age Correction -----*/
	key = get_ui_key(UI_AGECORR, UI_FIXED);
	value = map_get_str(&user_input, key);
	if (!isint(value)) {
		updateValidation(val, ERROR, "Age Correction [%s], "
				"expected of the form %s", 
				value, validMsg[AGECORRERR]);
	}

	/* ----- Check Inflation -----*/
	key = get_ui_key(UI_INFL, UI_FIXED);
	value = map_get_str(&user_input, key);
	if (!isfloat(value)) {
		updateValidation(val, ERROR, "Inflation [%s], "
				"expected of the form %s", 
				value, validMsg[FLOATERR]);
	}

	/* ----- Check Termination percentage -----*/
	key = get_ui_key(UI_TRM_PERCDEF, UI_FIXED);
	value = map_get_str(&user_input, key);
	if (!isfloat(value)) {
		updateValidation(val, ERROR, "Termination % [%s] (usually 1), "
				"expected of the form %s", 
				value, validMsg[FLOATERR]);
	}

	/* ----- Check DR 113 -----*/
	key = get_ui_key(UI_DR113, UI_FIXED);
	value = map_get_str(&user_input, key);
	if (!isfloat(value)) {
		updateValidation(val, WARNING, "DR $113 [%s], "
				"expected of the form %s", 
				value, validMsg[FLOATERR]);
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
void validateData(Validator val[static 1])
{
	if (val->status == OK) {
		//TODO
	}
}

void free_garbage(void)
{
	dates_arena_free();
}
