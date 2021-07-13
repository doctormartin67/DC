#include <pthread.h>
#include "DCProgram.h"
#include "inputerrors.h"

#define GLADEFILE "DCProgram.glade"

typedef struct {
    /* --- Data --- */
    char fname[PATH_MAX];
    char sheetname[64];
    char keycell[11];
    
    /* --- Assumptions --- */
    char DOC[16];
    char DR[16];
    char agecorr[16];
    char infl[16];
    char TRM_PercDef[16];
    char DR113[16];
    char SI[16];
    char SS[BUFSIZ]; /* This could be a large text describing salary scale with select cases */
    /* turnover still needs to be added !!! */

    /* --- Methodology --- */
    gint standard;
    gint assets;
    gint paragraph;
    gint PUCTUC;
    gint cashflows;
    gint evaluateDTH;
} UserInput;

/* This will be used as indices for the widget array */
enum {SHEETNAME, KEYCELL, DOC, DR, AGECORR, INFL, TRM_PERCDEF, DR113, FIXEDSIENTRY, SS, 
    STANDARD, ASSETS, PARAGRAPH, PUCTUC, CASHFLOWS, EVALUATEDTH, FIXEDSIRADIOBUTTON, RUNCHOICE,
    TESTCASEBOX, TESTCASE, OPENDCFILE, SAVEASDCFILE, OPENEXCELFILE, WINDOW, ASSWINDOW, MSGERR,
    FILENAME}; 

static const char *widgetname[] = {"sheetname", "keycell", "DOC", "DR", "agecorr", "infl", 
    "TRM_PercDef", "DR113", "fixedSIentry", "SS", "standard", "assets", "paragraph", "PUCTUC", 
    "cashflows", "evaluateDTH",  "fixedSIradiobutton", "runchoice", "testcasebox", "testcase", 
    "openDCFile", "saveasDCFile", "openExcelFile", "window", "asswindow", "MsgErr", "filename"};
static GtkWidget *widgets[128];
static GtkBuilder *builder;

static unsigned short running; /* determines whether program is running or not */
static DataSet *ds;
static pthread_t thrun;

static UserInput UI;
static char MsgErr[BUFSIZ];

extern void runmember(CurrentMember *cm);

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *, GtkWidget *);
void on_startstopbutton_clicked(GtkButton *, GtkWidget *);
void on_SIinterpreterbutton_clicked(GtkButton *b, gpointer *p);
void on_runchoice_changed(GtkComboBox *cb, gpointer *p);
gboolean on_asswindow_delete_event(GtkWidget *, GdkEvent *, gpointer);
void on_close_button_press_event(void);
void on_openDC_activate(GtkMenuItem *m);
void on_saveDC_activate(GtkMenuItem *m);
void on_saveasDC_activate(GtkMenuItem *m);
void on_LYfilechooserbutton_file_set(GtkFileChooserButton *, gpointer);

/* helper functions */
static GtkWidget *buildWidget(const char *);
static void *run(void *);
static void *runtc(void *pl);
static void setUIvals(void);
static void updateUI(void);
static Status validateUI(void);

void userinterface(DataSet *pds)
{
    unsigned short widgetcnt;

    ds = pds;

    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init(NULL, NULL);

    /* initialise widgets */
    builder = gtk_builder_new_from_file(GLADEFILE);
    widgetcnt = sizeof(widgetname)/sizeof(const char *);
    if (widgetcnt > sizeof(widgets)/sizeof(widgets[0]))
	errExit("[%s] [widgets] array is too small, increase the size in the code\n", __func__);

    for (int i = 0; i < widgetcnt; i++)
	widgets[i] = buildWidget(widgetname[i]);

    gtk_builder_connect_signals(builder, (void *)NULL);
    g_signal_connect(widgets[WINDOW], "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show(widgets[WINDOW]);

    /* Hand control over to the main loop. */
    /* It will continue to run until gtk_main_quit() is called or the application
       terminates. It sleeps and waits for signals to be emitted. */
    gtk_main();
}

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *rb, GtkWidget *w) {
    gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb));
    gtk_widget_set_sensitive(w, state);
}

void on_startstopbutton_clicked(GtkButton *b, GtkWidget *pl)
{
    printf("[%s] pressed\n", gtk_button_get_label(b));

    if (!running)
    {
	/* Set User Input values and check them before we start running */
	setUIvals();
	if (validateUI() == OK)
	{
	    running = TRUE;
	    int s = 0; /* used for error printing */
	    char *choice = gtk_combo_box_text_get_active_text(
		    GTK_COMBO_BOX_TEXT(widgets[RUNCHOICE]));
	    if (strcmp("Run one run", choice) == 0)
	    {
		// This needs updating when I start with reconciliation runs!!
		currrun = runNewRF; 
		s = pthread_create(&thrun, NULL, run, (void *)pl);
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
		s = pthread_create(&thrun, NULL, runtc, (void *)pl);
		if (s != 0)
		    errExitEN(s, "[%s] unable to create thread\n", __func__);

		s = pthread_detach(thrun);
		if (s != 0)
		    errExitEN(s, "[%s] unable to detach thread\n", __func__);
	    }
	    else if (strcmp("Run reconciliation", choice) == 0)
		printf("something else\n");
	    else
		errExit("[%s] should never reach here\n", __func__);
	}
	else
	{
	    GtkDialog *dialog;
	    dialog = GTK_DIALOG(widgets[MSGERR]);
	    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", MsgErr);
	    gtk_widget_show(GTK_WIDGET(dialog));
	    gtk_dialog_run(dialog); 
	    gtk_widget_hide(GTK_WIDGET(dialog));
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
    if (p != NULL) printf("unused pointer [%p]\n", p);

    char *choice; 
    if (cb == GTK_COMBO_BOX(widgets[RUNCHOICE]))
	choice = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widgets[RUNCHOICE]));
    else
	errExit("[%s] something went wrong with GtkComboBox\n", __func__);

    if (strcmp("Run test case", choice) == 0)
	gtk_widget_show_all(widgets[TESTCASEBOX]);
    else
	gtk_widget_hide(widgets[TESTCASEBOX]);
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
	    if (fread(&UI, sizeof(UI), 1, fp) != 1)
		printf("[%s] not a correct .dc file\n", filename);
	    else
		updateUI();
	    if (fclose(fp) == EOF) 
		errExit("[%s] unable to close file [%s]\n", __func__, filename);
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
}

void on_saveDC_activate(GtkMenuItem *m)
{
    printf("[%s] activated\n", gtk_menu_item_get_label(m));
}

void on_saveasDC_activate(GtkMenuItem *m)
{
    GtkDialog *dialog;
    gint res;
   
    setUIvals();
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
	    if (fwrite(&UI, sizeof(UI), 1, fp) != 1)
		errExit("[%s] unable to write to file [%s]\n", __func__, temp);
	    if (fclose(fp) == EOF) 
		errExit("[%s] unable to close file [%s]\n", __func__, filename);
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
    strcpy(UI.fname, filename);
    printf("Excel to run: [%s]\n", UI.fname);
    char temp[BUFSIZ];
    snprintf(temp, sizeof(temp), "File set to run:\n%s", UI.fname);
    gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), temp); 
}

/* helper functions */
static GtkWidget *buildWidget(const char *w)
{
    GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, w));
    if (widget == NULL) errExit("[%s] incorrect builder name\n", __func__);
    return widget;
}

static void *run(void *pl)
{
    char text[BUFSIZ];
    int tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[TESTCASE]))); 
    CurrentMember *cm = ds->cm;

    for (int i = 0; i < ds->membercnt; i++)
    {
	runmember(cm + i);
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
    CurrentMember *cm = ds->cm;

    runmember(cm + tc);
    snprintf(text, sizeof(text), "Test case %d has been run", tc + 1);
    gtk_label_set_text(GTK_LABEL(pl), text); 

    // create excel file to print results
    printtc(ds, tc);

    running = FALSE;
    return (void *)0;
}

static void setUIvals(void)
{
    snprintf(UI.sheetname, sizeof(UI.sheetname), "%s", 
	    gtk_entry_get_text(GTK_ENTRY(widgets[SHEETNAME])));
    snprintf(UI.keycell, sizeof(UI.keycell), "%s", 
	    gtk_entry_get_text(GTK_ENTRY(widgets[KEYCELL])));
    snprintf(UI.DOC, sizeof(UI.DOC), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[DOC])));
    snprintf(UI.DR, sizeof(UI.DR), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[DR])));
    snprintf(UI.agecorr, sizeof(UI.agecorr), "%s", 
	    gtk_entry_get_text(GTK_ENTRY(widgets[AGECORR])));
    snprintf(UI.infl, sizeof(UI.infl), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[INFL])));
    snprintf(UI.TRM_PercDef, sizeof(UI.TRM_PercDef), "%s", 
	    gtk_entry_get_text(GTK_ENTRY(widgets[TRM_PERCDEF])));
    snprintf(UI.DR113, sizeof(UI.DR113), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[DR113])));

    snprintf(UI.SI, sizeof(UI.SI), "%s", gtk_entry_get_text(GTK_ENTRY(widgets[FIXEDSIENTRY])));
    /* Text view is rather tedious to retrieve text from, this is why this seems so random */
    GtkTextBuffer *temp = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets[SS]));
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_offset(temp, &begin, (gint)0);
    gtk_text_buffer_get_iter_at_offset(temp, &end, (gint)-1);
    snprintf(UI.SS, sizeof(UI.SS), "%s", gtk_text_buffer_get_text(temp, &begin, &end, TRUE));

    UI.standard = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[STANDARD]));
    UI.assets = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[ASSETS]));
    UI.paragraph = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[PARAGRAPH]));
    UI.PUCTUC = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[PUCTUC]));
    UI.cashflows = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[CASHFLOWS]));
    UI.evaluateDTH = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets[EVALUATEDTH]));
}

static void updateUI(void)
{
    /* --- Data --- */
    char s[BUFSIZ];
    snprintf(s, sizeof(s), "File set to run:\n%s", UI.fname);
    gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), s); 
    gtk_entry_set_text(GTK_ENTRY(widgets[SHEETNAME]), UI.sheetname);
    gtk_entry_set_text(GTK_ENTRY(widgets[KEYCELL]), UI.keycell);

    /* --- Assumptions --- */
    gtk_entry_set_text(GTK_ENTRY(widgets[DOC]), UI.DOC);
    gtk_entry_set_text(GTK_ENTRY(widgets[DR]), UI.DR);
    gtk_entry_set_text(GTK_ENTRY(widgets[AGECORR]), UI.agecorr);
    gtk_entry_set_text(GTK_ENTRY(widgets[INFL]), UI.infl);
    gtk_entry_set_text(GTK_ENTRY(widgets[TRM_PERCDEF]), UI.TRM_PercDef);
    gtk_entry_set_text(GTK_ENTRY(widgets[DR113]), UI.DR113);
    gtk_entry_set_text(GTK_ENTRY(widgets[FIXEDSIENTRY]), UI.SI);

    GtkTextBuffer *temp = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets[SS]));
    gtk_text_buffer_set_text(temp, UI.SS, -1);

    /* --- Methodology --- */
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[STANDARD]), UI.standard);
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[ASSETS]), UI.assets);
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[PARAGRAPH]), UI.paragraph);
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[PUCTUC]), UI.PUCTUC);
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[CASHFLOWS]), UI.cashflows);
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[EVALUATEDTH]), UI.evaluateDTH);
}

static Status validateUI(void)
{
    char msg[BUFSIZ] = "";
    char temp[BUFSIZ];
    Status status = OK;

    /* ----- Check keycell -----*/
    int colcnt = 0;
    strcpy(temp, UI.keycell);
    upper(temp);
    strcpy(UI.keycell, temp);
    gtk_entry_set_text(GTK_ENTRY(widgets[KEYCELL]), UI.keycell);

    if (temp[0] < 'A' || temp[0] > 'Z')
    {
	setMsgErr(msg, "Key Cell", UI.keycell, CELLERR);
	status = ERROR;
    }
    else
    {
	char *pt = &temp[1];
	colcnt++;

	while (!isdigit(*pt))
	{
	    pt++;
	    colcnt++;
	}

	if (colcnt > 3)
	{
	    setMsgErr(msg, "Key Cell", UI.keycell, CELLERR);
	    status = ERROR;
	}

	if (*pt == '\0')
	{
	    setMsgErr(msg, "Key Cell", UI.keycell, CELLERR);
	    status = ERROR;
	}
	
	while (isdigit(*pt))
	    pt++;
	
	if (*pt != '\0')
	{
	    setMsgErr(msg, "Key Cell", UI.keycell, CELLERR);
	    status = ERROR;
	}
    }

    /* ----- Check DOC -----*/
    char *day, *month, *year;
    strcpy(temp, UI.DOC);

    day = strtok(temp, "/");
    month = strtok(NULL, "/");
    year = strtok(NULL, "");

    if (day == NULL || month == NULL || year == NULL)
    {
	setMsgErr(msg, "DOC", UI.DOC, DATEERR);
	status = ERROR;
    }
    else
    {
	Date *tempDate = newDate(0, atoi(year), atoi(month), atoi(day));
	if (tempDate == NULL)
	{
	    setMsgErr(msg, "DOC", UI.DOC, DATEERR);
	    status = ERROR;
	}
	free(tempDate);
    }

    /* ----- Check DR -----*/
    if (!isfloat(UI.DR))
    {
	setMsgErr(msg, "DR", UI.DR, FLOATERR);
	status = ERROR;
    }

    /* ----- Check Age Correction -----*/
    if (!isint(UI.agecorr))
    {
	setMsgErr(msg, "Age Correction", UI.agecorr, AGECORRERR);
	status = ERROR;
    }

    /* ----- Check Inflation -----*/
    if (!isfloat(UI.infl))
    {
	setMsgErr(msg, "Inflation", UI.infl, FLOATERR);
	status = ERROR;
    }

    /* ----- Check Termination percentage -----*/
    if (!isfloat(UI.TRM_PercDef))
    {
	setMsgErr(msg, "Termination % (usually 1)", UI.TRM_PercDef, FLOATERR);
	status = ERROR;
    }

    /* ----- Check DR 113 -----*/
    if (!isfloat(UI.DR113))
    {
	setMsgErr(msg, "DR $113", UI.DR113, FLOATERR);
	if (status == OK) status = WARNING;
    }

    /* ----- Check Salary Increase -----*/
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets[FIXEDSIRADIOBUTTON])))
    {
	if (!isfloat(UI.SI))
	{
	    setMsgErr(msg, "Salary Increase", UI.SI, FLOATERR);
	    status = ERROR;
	}
    }

    strcpy(MsgErr, msg);
    return status;
}
