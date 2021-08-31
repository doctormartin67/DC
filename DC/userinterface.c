#include <pthread.h>
#include "userinterface.h"
#include "libraryheader.h"
#include "errorexit.h"


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
	[FIXEDSIENTRY] = "fixedSIentry", [SS] = "SS", [STANDARD] = "standard",
	[ASSETS] = "assets", [PARAGRAPH] = "paragraph", [PUCTUC] = "PUCTUC",
	[CASHFLOWS] = "cashflows", [EVALUATEDTH] = "evaluateDTH",
	[FIXEDSIRADIOBUTTON] = "fixedSIradiobutton", [RUNCHOICE] = "runchoice",
	[TESTCASEBOX] = "testcasebox", [TESTCASE] = "testcase",
	[OPENDCFILE] = "openDCFile", [SAVEASDCFILE] = "saveasDCFile",
	[OPENEXCELFILE] = "openExcelFile", [WINDOW] = "window",
	[ASSWINDOW] = "asswindow", [MSGERR] = "MsgErr", [FILENAME] = "filename"
};

static GtkWidget *widgets[WIDGET_AMOUNT];
static GtkBuilder *builder;

static unsigned running; /* determines whether program is running or not */
static DataSet *ds;
static pthread_t thrun;

static UserInput UILY;
static Validator validatorLY;

extern void runmember(CurrentMember cm[static 1], UserInput UILY[static 1],
		UserInput UITY[static 1]);

void userinterface()
{
	gtk_init(0, 0);

	builder = gtk_builder_new_from_file(GLADEFILE);

	for (int i = 0; i < WIDGET_AMOUNT; i++)
		widgets[i] = buildWidget(widgetname[i]);

	gtk_builder_connect_signals(builder, 0);
	g_signal_connect(widgets[WINDOW], "destroy",
			G_CALLBACK(gtk_main_quit), 0);

	gtk_widget_show(widgets[WINDOW]);

	gtk_main();
}

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *rb, GtkWidget *w)
{
	gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb));
	gtk_widget_set_sensitive(w, state);
}

void on_startstopbutton_clicked(GtkButton *b, GtkWidget *pl)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));

	if (!running) {
		setUIvals(&UILY);

		validatorLY = (Validator) {0};
		validatorLY.status = OK;
		validateUI(&validatorLY, &UILY); 
		validateData(&validatorLY, &UILY);

		if (validatorLY.status != ERROR) {
			running = TRUE;
			int s = 0; /* used for error printing */
			gchar *choice = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widgets[RUNCHOICE]));
			if (strcmp("Run one run", choice) == 0) {
				// This needs updating when I start with reconciliation runs!!
				currrun = runNewRF; 
				s = pthread_create(&thrun, NULL, run, pl);
				if (s != 0)
					errExitEN(s, "[%s] unable to create thread\n", __func__);

				s = pthread_detach(thrun);
				if (s != 0)
					errExitEN(s, "[%s] unable to detach thread\n", __func__);
			}
			else if (strcmp("Run test case", choice) == 0)
			{
				// This needs updating when I start with reconciliation runs!!
				currrun = runNewRF; 
				s = pthread_create(&thrun, 0, runtc, pl);
				if (s != 0)
					errExitEN(s, "[%s] unable to create thread\n", __func__);

				s = pthread_detach(thrun);
				if (s != 0)
					errExitEN(s, "[%s] unable to detach thread\n", __func__);
			}
			else if (strcmp("Run reconciliation", choice) == 0)
				printf("something else\n");
			else
				errExit("should never reach here");

			g_free(choice);
		}
		else
		{
			char *MsgErr = setMsgbuf(&validatorLY);
			GtkDialog *dialog;
			dialog = GTK_DIALOG(widgets[MSGERR]);
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", MsgErr);
			gtk_widget_show(GTK_WIDGET(dialog));
			gtk_dialog_run(dialog); 
			gtk_widget_hide(GTK_WIDGET(dialog));

			freeDS(ds);
			ds = 0;
		}
	}
	else
		printf("Program is running, wait for it to end\n");
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
	if (GTK_COMBO_BOX(widgets[RUNCHOICE]) == cb)
		choice = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widgets[RUNCHOICE]));
	else
		errExit("something went wrong with GtkComboBox");

	if (strcmp("Run test case", choice) == 0)
		gtk_widget_show_all(widgets[TESTCASEBOX]);
	else
		gtk_widget_hide(widgets[TESTCASEBOX]);

	g_free(choice);
}

gboolean on_asswindow_delete_event(GtkWidget *w, GdkEvent *e, gpointer p)
{
	if (p != NULL) printf("unused pointer [%p]\n", p);
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
	GtkDialog *dialog;
	gint res;

	dialog = GTK_DIALOG(widgets[OPENDCFILE]);
	gtk_widget_show(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog); 

	if (res == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);
		printf("selected file [%s] to open\n", filename);
		FILE *fp = fopen(filename, "rb");
		if (fp != NULL)
		{
			if (fread(&UILY, sizeof(UILY), 1, fp) != 1)
				printf("[%s] not a correct .dc file\n", filename);
			else
				updateUI(&UILY);
			if (fclose(fp) == EOF) 
				errExit("unable to close file [%s]", filename);
		}
		else
		{
			fprintf(stderr, "Unable to open file [%s]\n", filename);
		}
		g_free(filename);
	}
	else
	{
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
	GtkDialog *dialog;
	gint res;

	setUIvals(&UILY);
	dialog = GTK_DIALOG(widgets[SAVEASDCFILE]);
	gtk_widget_show(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog); 

	if (res == GTK_RESPONSE_ACCEPT)
	{
		char temp[BUFSIZ];
		char *filename;
		char *p;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		p = filename = gtk_file_chooser_get_filename(chooser);

		if ((p = strstr(p, ".dc")) == NULL)
			snprintf(temp, sizeof(temp), "%s.dc", filename);
		else
		{ /* Check if last 3 character is ".dc" */
			while (*p != '\0')
				p++;
			p -= 3;
			if (strcmp(p, ".dc") == 0)
				strcpy(temp, filename);
			else
				snprintf(temp, sizeof(temp), "%s.dc", filename);
		}

		printf("selected file [%s] to save\n", temp);
		FILE *fp = fopen(temp, "wb");
		if (fp != NULL)
		{
			if (fwrite(&UILY, sizeof(UILY), 1, fp) != 1)
				errExit("unable to write to file [%s]", temp);
			if (fclose(fp) == EOF) 
				errExit("unable to close file [%s]", filename);
		}
		else
		{
			fprintf(stderr, "Unable to open file [%s]\n", temp);
		}
		g_free(filename);
	}
	else
	{
		printf("Cancelled [%s]\n", gtk_menu_item_get_label(m));
	}

	gtk_widget_hide(GTK_WIDGET(dialog));
}

void on_LYfilechooserbutton_file_set(GtkFileChooserButton *b, gpointer p)
{
	if (p != NULL) printf("unused pointer [%p]\n", p);
	printf("dialog [%s] closed\n", gtk_file_chooser_button_get_title(b));

	GtkDialog *dialog;
	char *filename;

	dialog = GTK_DIALOG(widgets[OPENEXCELFILE]);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	filename = gtk_file_chooser_get_filename(chooser);
	strcpy(UILY.fname, filename);
	printf("Excel to run: [%s]\n", UILY.fname);
	char temp[BUFSIZ];
	snprintf(temp, sizeof(temp), "File set to run:\n%s", UILY.fname);
	gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), temp); 
}

/* helper functions */
static GtkWidget *buildWidget(const char w[static 1])
{
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, w));
	if (0 == widget) errExit("incorrect builder name");
	return widget;
}

static void *run(void *pl)
{
	char text[BUFSIZ];
	int tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[TESTCASE]))); 
	CurrentMember *cm = ds->cm;

	for (int i = 0; i < ds->membercnt; i++)
	{
		/* this needs updating when we have a UITY!!! */
		runmember(cm + i, &UILY, &UILY);
		snprintf(text, sizeof(text), 
				"Progress: member %d out of %d members complete", i + 1, ds->membercnt);
		gtk_label_set_text(GTK_LABEL(pl), text); 
	}

	// create excel file to print results
	tc -= 1; // Index is one less than given test case
	printresults(ds);
	printtc(ds, tc);

	running = FALSE;
	return (void *)0;
}

static void *runtc(void *pl)
{
	char text[BUFSIZ];
	int tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[TESTCASE]))); 
	tc -= 1; // Index is one less than given test case
	CurrentMember *cm = ds->cm + tc;

	printf("testcase: %s chosen\n", cm->key);
	/* this needs updating when we have a UITY!!! */
	runmember(cm, &UILY, &UILY);
	snprintf(text, sizeof(text), "Test case %d has been run", tc + 1);
	gtk_label_set_text(GTK_LABEL(pl), text); 

	// create excel file to print results
	printtc(ds, tc);

	running = FALSE;
	return (void *)0;
}

static void setUIvals(UserInput UI[static 1])
{
	snprintf(UI->sheetname, sizeof(UI->sheetname), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[SHEETNAME])));
	snprintf(UI->keycell, sizeof(UI->keycell), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[KEYCELL])));
	snprintf(UI->DOC, sizeof(UI->DOC), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[DOC])));
	snprintf(UI->DR, sizeof(UI->DR), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[DR])));
	snprintf(UI->agecorr, sizeof(UI->agecorr), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[AGECORR])));
	snprintf(UI->infl, sizeof(UI->infl), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[INFL])));
	snprintf(UI->TRM_PercDef, sizeof(UI->TRM_PercDef), "%s", 
			gtk_entry_get_text(GTK_ENTRY(widgets[TRM_PERCDEF])));
	snprintf(UI->DR113, sizeof(UI->DR113), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[DR113])));

	snprintf(UI->SI, sizeof(UI->SI), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[FIXEDSIENTRY])));
	/* Text view is rather tedious to retrieve text from, this is why this seems so random */
	GtkTextBuffer *temp = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets[SS]));
	gchar *s = 0;
	GtkTextIter begin, end;
	gtk_text_buffer_get_iter_at_offset(temp, &begin, (gint)0);
	gtk_text_buffer_get_iter_at_offset(temp, &end, (gint)-1);
	s = gtk_text_buffer_get_text(temp, &begin, &end, TRUE);
	snprintf(UI->SS, sizeof(UI->SS), "%s", s);
	g_free(s);

	UI->standard = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[STANDARD]));
	UI->assets = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[ASSETS]));
	UI->paragraph = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[PARAGRAPH]));
	UI->PUCTUC = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[PUCTUC]));
	UI->cashflows = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[CASHFLOWS]));
	UI->evaluateDTH = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[EVALUATEDTH]));

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
	gtk_entry_set_text(GTK_ENTRY(widgets[FIXEDSIENTRY]), UI->SI);

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
	printf("SI [%s]\n", UI->SI);
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
	Date *tempDate = 0;

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
	if (gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(
					widgets[FIXEDSIRADIOBUTTON]))) {
		if (!isfloat(UI->SI)) {
			updateValidation(val, ERROR, "Salary Increase [%s], "
					"expected of the form %s", 
					UI->SI, validMsg[FLOATERR]);
		}
	}
}

static void validateData(Validator val[static 1], UserInput UI[static 1])
{
	if (val->status == OK) ds = createDS(val, UI); 
}
