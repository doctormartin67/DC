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
	[SHEETNAME] = "sheetname", [KEYCELL] = "keycell", [DOC] = "DOC",
	[DR] = "DR", [AGECORR] = "agecorr", [INFL] = "infl",
	[TRM_PERCDEF] = "TRM_PercDef", [DR113] = "DR113", [SS] = "SS",
	[STANDARD] = "standard", [ASSETS] = "assets",
	[PARAGRAPH] = "paragraph", [PUCTUC] = "PUCTUC",
	[CASHFLOWS] = "cashflows", [EVALUATEDTH] = "evaluateDTH",
	[RUNCHOICE] = "runchoice", [TESTCASEBOX] = "testcasebox",
	[TESTCASE] = "testcase", [OPENDCFILE] = "openDCFile",
	[SAVEASDCFILE] = "saveasDCFile", [OPENEXCELFILE] = "openExcelFile",
	[WINDOW] = "window", [INTERPRETERWINDOW] = "interpreterwindow",
	[MSGERR] = "MsgErr", [FILENAME] = "filename", [STARTSTOP] = "startstop"
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
	snprintf(UI->DOC, sizeof(UI->DOC), "%s", gtk_entry_get_text(
				GTK_ENTRY(widgets[DOC])));
	snprintf(UI->DR, sizeof(UI->DR), "%s", gtk_entry_get_text(
				GTK_ENTRY(widgets[DR])));
	snprintf(UI->agecorr, sizeof(UI->agecorr), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[AGECORR])));
	snprintf(UI->infl, sizeof(UI->infl), "%s", gtk_entry_get_text(
				GTK_ENTRY(widgets[INFL])));
	snprintf(UI->TRM_PercDef, sizeof(UI->TRM_PercDef), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[TRM_PERCDEF])));
	snprintf(UI->DR113, sizeof(UI->DR113), "%s", gtk_entry_get_text(
				GTK_ENTRY(widgets[DR113])));

	/* 
	 * Text view is rather tedious to retrieve text from,
	 * this is why this seems so random
	 */
	GtkTextBuffer *temp = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(widgets[SS]));
	gchar *s = 0;
	GtkTextIter begin, end;
	gtk_text_buffer_get_iter_at_offset(temp, &begin, 0);
	gtk_text_buffer_get_iter_at_offset(temp, &end, -1);
	s = gtk_text_buffer_get_text(temp, &begin, &end, TRUE);
	snprintf(UI->SS, sizeof(UI->SS), "%s", s);
	g_free(s);

	UI->standard = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[STANDARD]));
	UI->assets = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[ASSETS]));
	UI->paragraph = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[PARAGRAPH]));
	UI->PUCTUC = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[PUCTUC]));
	UI->cashflows = gtk_combo_box_get_active(
			GTK_COMBO_BOX(widgets[CASHFLOWS]));
	UI->evaluateDTH = gtk_combo_box_get_active(
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
	gtk_entry_set_text(GTK_ENTRY(widgets[DOC]), UI->DOC);
	gtk_entry_set_text(GTK_ENTRY(widgets[DR]), UI->DR);
	gtk_entry_set_text(GTK_ENTRY(widgets[AGECORR]), UI->agecorr);
	gtk_entry_set_text(GTK_ENTRY(widgets[INFL]), UI->infl);
	gtk_entry_set_text(GTK_ENTRY(widgets[TRM_PERCDEF]), UI->TRM_PercDef);
	gtk_entry_set_text(GTK_ENTRY(widgets[DR113]), UI->DR113);

	GtkTextBuffer *temp = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets[SS]));
	gtk_text_buffer_set_text(temp, UI->SS, -1);

	/* --- Methodology --- */
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[STANDARD]), UI->standard);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[ASSETS]), UI->assets);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[PARAGRAPH]), UI->paragraph);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[PUCTUC]), UI->PUCTUC);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[CASHFLOWS]), UI->cashflows);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[EVALUATEDTH]), UI->evaluateDTH);
}

void print_user_input(struct user_input UI[static 1])
{
	/* --- Data --- */
	printf("file name [%s]\n", UI->fname);
	printf("sheet name [%s]\n", UI->sheetname);
	printf("keycell [%s]\n", UI->keycell);

	/* --- Assumptions --- */
	printf("DOC [%s]\n", UI->DOC);
	printf("DR [%s]\n", UI->DR);
	printf("agecorr [%s]\n", UI->agecorr);
	printf("infl [%s]\n", UI->infl);
	printf("TRM_PercDef [%s]\n", UI->TRM_PercDef);
	printf("DR113 [%s]\n", UI->DR113);
	printf("SS [%s]\n", UI->SS);

	/* --- Methodology --- */
	printf("standard [%d]\n", UI->standard); 
	printf("assets [%d]\n", UI->assets); 
	printf("paragraph [%d]\n", UI->paragraph); 
	printf("PUCTUC [%d]\n", UI->PUCTUC); 
	printf("cashflows [%d]\n", UI->cashflows); 
	printf("evaluateDTH [%d]\n", UI->evaluateDTH); 
}

void validateUI(Validator val[static 1], struct user_input UI[static 1])
{
	size_t len = 0;
	register unsigned colcnt = 0;
	char temp[strlen(UI->DOC) + 1];
	char *kc = UI->keycell;
	char *pt = 0;
	char *day = 0, *month = 0, *year = 0;
	const char *tc = 0;
	struct date *tempDate = 0;
	struct casetree *ct = 0;

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

	snprintf(temp, sizeof(temp), "%s", UI->DOC);

	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	if (0 == day || 0 == month || 0 == year) {
		updateValidation(val, ERROR, "DOC [%s], "
				"expected of the form %s",
				UI->DOC, validMsg[DATEERR]);
	} else {
		if (!isint(day) || !isint(month) || !isint(year)) {
			updateValidation(val, ERROR, "DOC [%s], expected of "
					"the form %s", 
					UI->DOC, validMsg[DATEERR]);
		}

		tempDate = newDate(0, atoi(year), atoi(month), atoi(day));
		if (0 == tempDate) {
			updateValidation(val, ERROR, "DOC [%s], "
					"expected of the form %s", 
					UI->DOC, validMsg[DATEERR]);
		}
		free(tempDate);
	}

	/* ----- Check DR -----*/
	if (!isfloat(UI->DR)) {
		updateValidation(val, ERROR, "DR [%s], "
				"expected of the form %s",
				UI->DR, validMsg[FLOATERR]);
	}

	/* ----- Check Age Correction -----*/
	if (!isint(UI->agecorr)) {
		updateValidation(val, ERROR, "Age Correction [%s], "
				"expected of the form %s", 
				UI->agecorr, validMsg[AGECORRERR]);
	}

	/* ----- Check Inflation -----*/
	if (!isfloat(UI->infl)) {
		updateValidation(val, ERROR, "Inflation [%s], "
				"expected of the form %s", 
				UI->infl, validMsg[FLOATERR]);
	}

	/* ----- Check Termination percentage -----*/
	if (!isfloat(UI->TRM_PercDef)) {
		updateValidation(val, ERROR, "Termination % [%s] (usually 1), "
				"expected of the form %s", 
				UI->TRM_PercDef, validMsg[FLOATERR]);
	}

	/* ----- Check DR 113 -----*/
	if (!isfloat(UI->DR113)) {
		updateValidation(val, WARNING, "DR $113 [%s], "
				"expected of the form %s", 
				UI->DR113, validMsg[FLOATERR]);
	}

	/* ----- Check Salary Increase -----*/
	ct = plantTree(strclean(UI->SS));
	if (NOERR != getterrno()) {
		updateValidation(val, ERROR, "Salary Increase interpreter: %s",
				strterror(getterrno()));
		setterrno(NOERR);
	}

	chopTree(ct);
	ct = 0;

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
