#include "userinterface.h"
#include "libraryheader.h"
#include "errorexit.h"

#define NOT_RUNNING 01
#define RUNNING 02
#define INTERRUPTED 04

const char *const validMsg[ERR_AMOUNT] = {
	[DATEERR] = "[dd/mm/yyyy]", 
	[FLOATERR] = "^[+-]?[0-9]+\\.?[0-9]*$", 
	[AGECORRERR] = "^[+-]?[0-9][0-9]?$", 
	[CELLERR] = "^[A-Z][A-Z]?[A-Z]?[1-9][0-9]*$"
};

const char *const widgetname[WIDGET_AMOUNT] = {
	[SHEETNAME] = "sheetname", [KEYCELL] = "keycell", [DOC] = "DOC",
	[DR] = "DR", [AGECORR] = "agecorr", [INFL] = "infl",
	[TRM_PERCDEF] = "TRM_PercDef", [DR113] = "DR113",
	[SS] = "SS", [STANDARD] = "standard",
	[ASSETS] = "assets", [PARAGRAPH] = "paragraph", [PUCTUC] = "PUCTUC",
	[CASHFLOWS] = "cashflows", [EVALUATEDTH] = "evaluateDTH",
	[RUNCHOICE] = "runchoice",
	[TESTCASEBOX] = "testcasebox", [TESTCASE] = "testcase",
	[OPENDCFILE] = "openDCFile", [SAVEASDCFILE] = "saveasDCFile",
	[OPENEXCELFILE] = "openExcelFile", [WINDOW] = "window",
	[ASSWINDOW] = "asswindow", [MSGERR] = "MsgErr",
	[FILENAME] = "filename", [STARTSTOP] = "startstop"
};

struct gui_data {
	char *s;
	gpointer pl;
};

static GtkWidget *widgets[WIDGET_AMOUNT];
static GtkBuilder *builder;

static _Atomic unsigned run_state = NOT_RUNNING;

static UserInput UILY;
static Validator validatorLY;

/* helper functions */
static GtkWidget *buildWidget(const char *);
static gpointer run(gpointer pl);
static gpointer runtc(gpointer pl);
static gpointer stoprun(gpointer data);
gboolean update_gui(gpointer data);
static void setUIvals(UserInput *UI);
static void updateUI(UserInput *UI);
static void printUI(UserInput *UI);
static void validateUI(Validator *, UserInput *);
static void validateData(Validator *, UserInput *);

extern void runmember(CurrentMember cm[static 1], UserInput UILY[static 1],
		UserInput UITY[static 1]);

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

/* signal functions */
void on_startstopbutton_clicked(GtkButton *b, GtkWidget *pl)
{
	char *MsgErr = 0;
	gchar *choice = 0;
	GtkDialog *dialog = 0;

	if (run_state & NOT_RUNNING) {

		run_state = RUNNING;
		gtk_image_set_from_icon_name(GTK_IMAGE(widgets[STARTSTOP]),
				"media-playback-stop", GTK_ICON_SIZE_BUTTON);
		setUIvals(&UILY);
		validatorLY = (Validator) {0};
		validatorLY.status = OK;
		validateUI(&validatorLY, &UILY); 
		validateData(&validatorLY, &UILY);

		if (validatorLY.status != ERROR) {
			choice = gtk_combo_box_text_get_active_text(
					GTK_COMBO_BOX_TEXT(
						widgets[RUNCHOICE]));
			if (strcmp("Run one run", choice) == 0) {
				// This needs updating when I start with reconciliation runs!!
				currrun = runNewRF; 
				g_thread_new("run", run, pl);
			} else if (strcmp("Run test case", choice) == 0) {
				// This needs updating when I start with reconciliation runs!!
				currrun = runNewRF; 
				g_thread_new("runtc", runtc, pl);
			} else if (strcmp("Run reconciliation", choice) == 0) {
				printf("something else\n");
			} else
				die("should never reach here");

			g_free(choice);
		} else {
			MsgErr = setMsgbuf(&validatorLY);
			dialog = GTK_DIALOG(widgets[MSGERR]);

			gtk_message_dialog_format_secondary_text(
					GTK_MESSAGE_DIALOG(dialog), 
					"%s", MsgErr);

			gtk_widget_show(GTK_WIDGET(dialog));
			gtk_dialog_run(dialog); 
			gtk_widget_hide(GTK_WIDGET(dialog));
			run_state = NOT_RUNNING;
			gtk_image_set_from_icon_name(
					GTK_IMAGE(widgets[STARTSTOP]),
					"media-playback-start",
					GTK_ICON_SIZE_BUTTON);
		}
	} else {
		run_state = INTERRUPTED;
	}
}

void on_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_runchoice_changed(GtkComboBox *cb, gpointer *p)
{
	if (0 != p) printf("unused pointer [%p]\n", p);

	gchar *choice = 0;
	if (GTK_COMBO_BOX(widgets[RUNCHOICE]) == cb) {
		choice = gtk_combo_box_text_get_active_text(
				GTK_COMBO_BOX_TEXT(widgets[RUNCHOICE]));
	} else
		die("something went wrong with GtkComboBox");

	if (strcmp("Run test case", choice) == 0)
		gtk_widget_show_all(widgets[TESTCASEBOX]);
	else
		gtk_widget_hide(widgets[TESTCASEBOX]);

	g_free(choice);
}

gboolean on_asswindow_delete_event(GtkWidget *w, GdkEvent *e, gpointer p)
{
	if (0 != p) printf("unused pointer [%p]\n", p);
	printf("GdkEventType [%d]\n", gdk_event_get_event_type(e));

	gtk_widget_hide(w);
	return TRUE;
}

void on_close_button_press_event(void)
{
	gtk_main_quit();
}

void on_openDC_activate(GtkMenuItem *m)
{
	FILE *fp = 0;
	char *filename = 0;
	gint res = 0;
	GtkDialog *dialog = 0;
	GtkFileChooser *chooser = 0;

	dialog = GTK_DIALOG(widgets[OPENDCFILE]);
	gtk_widget_show(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog); 

	if (res == GTK_RESPONSE_ACCEPT) {
		chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);
		printf("selected file [%s] to open\n", filename);

		fp = fopen(filename, "rb");
		if (0 == fp) die("Unable to open file [%s]", filename);

		if (fread(&UILY, sizeof(UILY), 1, fp) != 1)
			die("[%s] not a correct .dc file\n", filename);

		updateUI(&UILY);

		if (fclose(fp) == EOF) 
			die("unable to close file [%s]", filename);

		g_free(filename);
		filename = 0;
	} else {
		printf("Cancelled [%s]\n", gtk_menu_item_get_label(m));
	}

	gtk_widget_hide(GTK_WIDGET(dialog));
	printUI(&UILY);
}

void on_saveDC_activate(GtkMenuItem *m)
{
	printf("[%s] activated\n", gtk_menu_item_get_label(m));
}

void on_saveasDC_activate(GtkMenuItem *m)
{
	FILE *fp = 0;
	char tmp[BUFSIZ];
	char *filename = 0;
	char *p = 0;
	GtkDialog *dialog = 0;
	GtkFileChooser *chooser = 0;
	gint res = 0;

	setUIvals(&UILY);
	dialog = GTK_DIALOG(widgets[SAVEASDCFILE]);
	gtk_widget_show(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog); 

	if (res == GTK_RESPONSE_ACCEPT) {
		chooser = GTK_FILE_CHOOSER(dialog);
		p = filename = gtk_file_chooser_get_filename(chooser);

		if (0 == (p = strstr(p, ".dc"))) {
			snprintf(tmp, sizeof(tmp), "%s.dc", filename);
		} else {
			while (*p != '\0') p++;
			p -= 3;
			if (strcmp(p, ".dc") == 0)
				snprintf(tmp, sizeof(tmp), "%s", filename);
			else
				snprintf(tmp, sizeof(tmp), "%s.dc", filename);
		}
		printf("selected file [%s] to save\n", tmp);

		fp = fopen(tmp, "wb");
		if (0 == fp) die("Unable to open file [%s]\n", tmp);

		if (fwrite(&UILY, sizeof(UILY), 1, fp) != 1)
			die("unable to write to file [%s]", tmp);
		if (fclose(fp) == EOF) 
			die("unable to close file [%s]", filename);

		g_free(filename);
		p = filename = 0;
	} else {
		printf("Cancelled [%s]\n", gtk_menu_item_get_label(m));
	}

	gtk_widget_hide(GTK_WIDGET(dialog));
}

void on_LYfilechooserbutton_file_set(GtkFileChooserButton *b, gpointer p)
{
	if (0 != p) printf("unused pointer [%p]\n", p);
	printf("dialog [%s] closed\n", gtk_file_chooser_button_get_title(b));

	GtkDialog *dialog = 0;
	char *filename = 0;
	char tmp[BUFSIZ];

	dialog = GTK_DIALOG(widgets[OPENEXCELFILE]);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	filename = gtk_file_chooser_get_filename(chooser);

	snprintf(UILY.fname, sizeof(UILY.fname), "%s", filename);
	snprintf(tmp, sizeof(tmp), "File set to run:\n%s", UILY.fname);

	printf("Excel to run: [%s]\n", UILY.fname);
	gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), tmp); 
	g_free(filename);
}

/* helper functions */
static GtkWidget *buildWidget(const char w[static 1])
{
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, w));
	if (0 == widget) die("incorrect builder name");
	return widget;
}

static gpointer run(gpointer pl)
{
	unsigned tc = 0;
	DataSet *ds = 0;
	CurrentMember *cm = 0;
	char text[BUFSIZ];
	struct gui_data gd = {"Preparing data...", pl};
	update_gui(&gd);

	tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[TESTCASE])));
	tc -= 1; // Index is one less than given test case
	ds = createDS(0, &UILY);
	cm = ds->cm;

	for (unsigned i = 0; i < ds->membercnt; i++) {
		if (run_state & INTERRUPTED) {
			freeDS(ds);
			return stoprun(&gd);
		}
		/* this needs updating when we have a UITY!!! */
		runmember(cm + i, &UILY, &UILY);
		snprintf(text, sizeof(text), "Progress: member %u out of %u "
				"members complete", i + 1, ds->membercnt);
		gd.s = text;
		update_gui(&gd);
	}

	printresults(ds);
	printtc(ds, tc);

	freeDS(ds);
	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name( GTK_IMAGE(widgets[STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	return 0;
}

static gpointer runtc(gpointer pl)
{
	unsigned tc = 0;
	DataSet *ds = 0;
	CurrentMember *cm = 0;
	char text[BUFSIZ];
	struct gui_data gd = {"Preparing data...", pl};
	update_gui(&gd);

	tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[TESTCASE])));
	tc -= 1; // Index is one less than given test case
	ds = createDS(0, &UILY);
	cm = ds->cm + tc;

	printf("testcase: %s chosen\n", cm->key);
	if (run_state & INTERRUPTED) {
		freeDS(ds);
		return stoprun(&gd);
	}
	/* this needs updating when we have a UITY!!! */
	runmember(cm, &UILY, &UILY);
	snprintf(text, sizeof(text), "Test case %u has been run", tc + 1);
	gd.s = text;
	update_gui(&gd);

	printtc(ds, tc);

	freeDS(ds);
	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name( GTK_IMAGE(widgets[STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	return 0;
}

static gpointer stoprun(gpointer data)
{
	struct gui_data *gd = data;
	gd->s = "Progress: stopped";
	update_gui(gd);
	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name( GTK_IMAGE(widgets[STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	return 0;
}

gboolean update_gui(gpointer data) {
	struct gui_data *gd = data;
	gtk_label_set_text(GTK_LABEL(gd->pl), gd->s);
	return FALSE;
}

static void setUIvals(UserInput UI[static 1])
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

	printUI(UI);
}

static void updateUI(UserInput UI[static 1])
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

static void printUI(UserInput UI[static 1])
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

static void validateUI(Validator val[static 1], UserInput UI[static 1])
{
	size_t len = 0;
	register unsigned colcnt = 0;
	char temp[strlen(UI->DOC) + 1];
	char *kc = UI->keycell;
	char *pt = 0;
	char *day = 0, *month = 0, *year = 0;
	const char *tc = 0;
	struct date *tempDate = 0;

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
static void validateData(Validator val[static 1], UserInput UI[static 1])
{
	DataSet *ds = 0;
	if (val->status == OK) {
		ds = createDS(val, UI); 
		freeDS(ds);
	}
}
