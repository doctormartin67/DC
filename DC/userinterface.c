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

const char *const ui_var_names[UI_AMOUNT] = {
	[UI_SS] = "Salary Scale",
	[UI_TURNOVER] = "Turnover",
	[UI_RETX] = "Retirement Probability",
	[UI_NRA] = "Normal Retirement Age"
};

GtkWidget *widgets[WIDGET_AMOUNT];
static GtkBuilder *builder;

static struct user_input UILY;
static struct user_input UITY;

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

struct user_input *get_user_input(unsigned ui)
{
	if (USER_INPUT_LY == ui) {
		return &UILY;
	} else if (USER_INPUT_TY == ui) { 
		return &UITY;
	} else die("ui not equal to USER_INPUT_LY or USER_INPUT_TY");

	return 0;
}

void set_user_input(struct user_input UI[static 1])
{
	snprintf(UI->sheetname, sizeof(UI->sheetname), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[SHEETNAME])));
	snprintf(UI->keycell, sizeof(UI->keycell), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[KEYCELL])));
	snprintf(UI->var[UI_DOC], sizeof(UI->var[UI_DOC]), "%s",
			gtk_entry_get_text(GTK_ENTRY(widgets[W_DOC_LY])));
	snprintf(UI->var[UI_DR], sizeof(UI->var[UI_DR]), "%s",
			gtk_entry_get_text(GTK_ENTRY(widgets[DR])));
	snprintf(UI->var[UI_AGECORR], sizeof(UI->var[UI_AGECORR]), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[AGECORR])));
	snprintf(UI->var[UI_INFL], sizeof(UI->var[UI_INFL]), "%s",
			gtk_entry_get_text(GTK_ENTRY(widgets[INFL])));
	snprintf(UI->var[UI_TRM_PERCDEF], sizeof(UI->var[UI_TRM_PERCDEF]),
			"%s",
			gtk_entry_get_text(GTK_ENTRY(widgets[TRM_PERCDEF])));
	snprintf(UI->var[UI_DR113], sizeof(UI->var[UI_DR113]), "%s",
			gtk_entry_get_text(GTK_ENTRY(widgets[DR113])));

	UI->method[METH_STANDARD] = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[STANDARD]));
	UI->method[METH_ASSETS] = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[W_ASSETS_LY]));
	UI->method[METH_DBO] = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[PUCTUC]));
	UI->method[METH_MAXPUCTUC] = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[MAXPUCTUC]));
	UI->method[METH_MAXERCONTR] = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[MAXERCONTR]));
	UI->method[METH_EVALDTH] = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[EVALUATEDTH]));

	print_user_input(UI);
}

void update_user_interface(struct user_input UI[static 1])
{
	/* --- Data --- */
	char s[BUFSIZ];
	snprintf(s, sizeof(s), "File set to run:\n%s", UI->fname);
	gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), s); 
	gtk_entry_set_text(GTK_ENTRY(widgets[SHEETNAME]), UI->sheetname);
	gtk_entry_set_text(GTK_ENTRY(widgets[KEYCELL]), UI->keycell);

	/* --- Assumptions --- */
	gtk_entry_set_text(GTK_ENTRY(widgets[W_DOC_LY]), UI->var[UI_DOC]);
	gtk_entry_set_text(GTK_ENTRY(widgets[DR]), UI->var[UI_DR]);
	gtk_entry_set_text(GTK_ENTRY(widgets[AGECORR]), UI->var[UI_AGECORR]);
	gtk_entry_set_text(GTK_ENTRY(widgets[INFL]), UI->var[UI_INFL]);
	gtk_entry_set_text(GTK_ENTRY(widgets[TRM_PERCDEF]),
			UI->var[UI_TRM_PERCDEF]);
	gtk_entry_set_text(GTK_ENTRY(widgets[DR113]), UI->var[UI_DR113]);

	/* --- Methodology --- */
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[STANDARD]),
			UI->method[METH_STANDARD]);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[W_ASSETS_LY]),
			UI->method[METH_ASSETS]);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[PUCTUC]),
			UI->method[METH_DBO]);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[MAXPUCTUC]),
			UI->method[METH_MAXPUCTUC]);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[MAXERCONTR]),
			UI->method[METH_MAXERCONTR]);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[EVALUATEDTH]),
			UI->method[METH_EVALDTH]);
}

void print_user_input(struct user_input UI[static 1])
{
	/* --- Data --- */
	printf("file name [%s]\n", UI->fname);
	printf("sheet name [%s]\n", UI->sheetname);
	printf("keycell [%s]\n", UI->keycell);

	/* --- Assumptions --- */
	printf("DOC [%s]\n", UI->var[UI_DOC]);
	printf("DR [%s]\n", UI->var[UI_DR]);
	printf("agecorr [%s]\n", UI->var[UI_AGECORR]);
	printf("infl [%s]\n", UI->var[UI_INFL]);
	printf("TRM_PercDef [%s]\n", UI->var[UI_TRM_PERCDEF]);
	printf("DR113 [%s]\n", UI->var[UI_DR113]);

	for (unsigned i = 0; i < UI_AMOUNT; i++) {
		printf("%s [%s]\n", ui_var_names[i], UI->var[i]);
	}
	/* --- Methodology --- */
	printf("standard [%d]\n", UI->method[METH_STANDARD]); 
	printf("assets [%d]\n", UI->method[METH_ASSETS]); 
	printf("PUCTUC [%d]\n", UI->method[METH_DBO]); 
	printf("max(PUC, TUC) [%d]\n", UI->method[METH_MAXPUCTUC]); 
	printf("max(SC, Contr A) [%d]\n", UI->method[METH_MAXERCONTR]);
	printf("evaluateDTH [%d]\n", UI->method[METH_EVALDTH]); 
}

void validateUI(Validator val[static 1], struct user_input UI[static 1])
{
	size_t len = 0;
	register unsigned colcnt = 0;
	char temp[strlen(UI->var[UI_DOC]) + 1];
	char *kc = UI->keycell;
	char *pt = 0;
	char *day = 0, *month = 0, *year = 0;
	const char *tc = 0;
	struct date *tempDate = 0;
	struct casetree *ct = 0;

	/* ----- Check File -----*/
	if (!*UI->fname)
		updateValidation(val, ERROR, "No file selected to run");

	/* ----- Check keycell -----*/
	upper(kc);
	len = strlen(kc);
	gtk_entry_set_text(GTK_ENTRY(widgets[KEYCELL]), UI->keycell);

	if (kc[0] < 'A' || kc[0] > 'Z') {
		updateValidation(val, ERROR, "KEY cell [%s], "
				"expected of the form %s", 
				UI->keycell, validMsg[CELLERR]);
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
					UI->keycell, validMsg[CELLERR]);
		}

		if ('\0' == *pt) {
			updateValidation(val, ERROR, "KEY cell [%s], "
					"expected of the form %s", 
					UI->keycell, validMsg[CELLERR]);
		}

		while (isdigit(*pt)) pt++;

		if ('\0' != *pt) {
			updateValidation(val, ERROR, "KEY cell [%s], "
					"expected of the form %s", 
					UI->keycell, validMsg[CELLERR]);
		}
	} else if (1 == len)
		updateValidation(val, ERROR, "KEY cell [%s], "
				"expected of the form %s", 
				UI->keycell, validMsg[CELLERR]);

	/* ----- Check DOC -----*/

	snprintf(temp, sizeof(temp), "%s", UI->var[UI_DOC]);

	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	if (0 == day || 0 == month || 0 == year) {
		updateValidation(val, ERROR, "DOC [%s], "
				"expected of the form %s",
				UI->var[UI_DOC], validMsg[DATEERR]);
	} else {
		if (!isint(day) || !isint(month) || !isint(year)) {
			updateValidation(val, ERROR, "DOC [%s], expected of "
					"the form %s", 
					UI->var[UI_DOC], validMsg[DATEERR]);
		}

		tempDate = newDate(0, atoi(year), atoi(month), atoi(day));
		if (0 == tempDate) {
			updateValidation(val, ERROR, "DOC [%s], "
					"expected of the form %s", 
					UI->var[UI_DOC], validMsg[DATEERR]);
		}
		free(tempDate);
	}

	/* ----- Check DR -----*/
	if (!isfloat(UI->var[UI_DR])) {
		updateValidation(val, ERROR, "DR [%s], "
				"expected of the form %s",
				UI->var[UI_DR], validMsg[FLOATERR]);
	}

	/* ----- Check Age Correction -----*/
	if (!isint(UI->var[UI_AGECORR])) {
		updateValidation(val, ERROR, "Age Correction [%s], "
				"expected of the form %s", 
				UI->var[UI_AGECORR], validMsg[AGECORRERR]);
	}

	/* ----- Check Inflation -----*/
	if (!isfloat(UI->var[UI_INFL])) {
		updateValidation(val, ERROR, "Inflation [%s], "
				"expected of the form %s", 
				UI->var[UI_INFL], validMsg[FLOATERR]);
	}

	/* ----- Check Termination percentage -----*/
	if (!isfloat(UI->var[UI_TRM_PERCDEF])) {
		updateValidation(val, ERROR, "Termination % [%s] (usually 1), "
				"expected of the form %s", 
				UI->var[UI_TRM_PERCDEF], validMsg[FLOATERR]);
	}

	/* ----- Check DR 113 -----*/
	if (!isfloat(UI->var[UI_DR113])) {
		updateValidation(val, WARNING, "DR $113 [%s], "
				"expected of the form %s", 
				UI->var[UI_DR113], validMsg[FLOATERR]);
	}

	/* ----- Check Tree Variables -----*/
	for (unsigned i = 0; i < UI_AMOUNT; i++) {
		ct = plantTree(strclean(UI->var[i]));
		if (NOERR != getterrno()) {
			updateValidation(val, ERROR, "%s: %s", ui_var_names[i],
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
void validateData(Validator val[static 1], struct user_input UI[static 1])
{
	DataSet *ds = 0;
	if (val->status == OK) {
		ds = createDS(val, UI); 
		freeDS(ds);
	}
}
