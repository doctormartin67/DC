#include <pthread.h>
#include "DCProgram.h"

#define GLADEFILE "DCProgram.glade"

typedef struct {
    /* --- Assumptions --- */
    char DOC[16];
    char DR[16];
    char agecorr[16];
    char infl[16];
    char TRM_PercDef[16];
    char DR113[16];
    char SS[BUFSIZ]; /* This could be a large text describing salary scale with select cases */
    /* turnover still needs to be added !!! */

    /* --- Methodology --- */
    char standard[16];
    char assets[16];
    char paragraph[16];
    char PUCTUC[16];
    char cashflows[16];
    char evaluateDTH[16];
} UserInput;

typedef enum {dateErr, floatErr, agecorrErr} Err;

static const char *validMsg[] = {"[dd/mm/yyyy]", "^[+-]?[0-9]+\\.?[0-9]*", "[+-]?[0-9][0-9]?"};

static unsigned short running; /* determines whether program is running or not */
static DataSet *ds;
static pthread_t thrun;

static UserInput UI;
static GtkEntry *DOC;
static GtkEntry *DR;
static GtkEntry *agecorr;
static GtkEntry *infl;
static GtkEntry *TRM_PercDef;
static GtkEntry *DR113;
static GtkEntry *fixedSIentry;
static GtkTextView *SS;
static GtkComboBoxText *standard;
static GtkComboBoxText *assets;
static GtkComboBoxText *paragraph;
static GtkComboBoxText *PUCTUC;
static GtkComboBoxText *cashflows;
static GtkComboBoxText *evaluateDTH;

static GtkToggleButton *fixedSIradiobutton;

extern void runmember(CurrentMember *cm);
static GtkWidget *runchoice; /* used for combo box text to choose which run option */
static GtkWidget *testcasebox; /* used to hide and show box with test case input */
static GtkEntry *testcase; /* this is the actual test case to run */

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *, GtkWidget *);
void on_startstopbutton_clicked(GtkButton *, GtkWidget *);
void on_SIinterpreterbutton_clicked(GtkButton *b, gpointer *p);
void on_runchoice_changed(GtkComboBox *cb, gpointer *p);
gboolean on_asswindow_delete_event(GtkWidget *, GdkEvent *, gpointer);
void on_close_button_press_event(void);

/* helper functions */
static void *run(void *);
static void *runtc(void *pl);
void setUIvals(void);
unsigned short validateUI(void);
void setMsgErr(char msg[], const char *input, const char UIs[], Err err);

void userinterface(DataSet *pds) {
    GtkBuilder *builder;
    GtkWidget *window, *asswindow;

    ds = pds;

    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init(NULL, NULL);

    builder = gtk_builder_new_from_file(GLADEFILE);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

    fixedSIradiobutton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, 
		"fixedSIradiobutton"));

    /* what to run */
    runchoice = GTK_WIDGET(gtk_builder_get_object(builder, "runchoice"));
    testcasebox = GTK_WIDGET(gtk_builder_get_object(builder, "testcasebox"));
    testcase = GTK_ENTRY(gtk_builder_get_object(builder, "testcase"));

    /* User Input */
    DOC = GTK_ENTRY(gtk_builder_get_object(builder, "DOC"));
    DR = GTK_ENTRY(gtk_builder_get_object(builder, "DR"));
    agecorr = GTK_ENTRY(gtk_builder_get_object(builder, "agecorr"));
    infl = GTK_ENTRY(gtk_builder_get_object(builder, "infl"));
    TRM_PercDef = GTK_ENTRY(gtk_builder_get_object(builder, "TRM_PercDef"));
    DR113 = GTK_ENTRY(gtk_builder_get_object(builder, "DR113"));
    fixedSIentry = GTK_ENTRY(gtk_builder_get_object(builder, "fixedSIentry"));
    SS = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "SS"));
    standard = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "standard"));
    assets = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "assets"));
    paragraph = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "paragraph"));
    PUCTUC = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "PUCTUC"));
    cashflows = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "cashflows"));
    evaluateDTH = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "evaluateDTH"));

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_builder_connect_signals(builder, (void *)NULL);

    gtk_widget_show(window);

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
    if (!running)
    {
	/* Set User Input values and check them before we start running */
	setUIvals();
	if (validateUI())
	{
	    running = TRUE;
	    int s = 0; /* used for error printing */
	    char *choice = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(runchoice));
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
    }
    else
	printf("Program is running, wait for it to end\n");
}

void on_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
    gtk_widget_show_all(GTK_WIDGET(w));
}

void on_runchoice_changed(GtkComboBox *cb, gpointer *p)
{
    char *choice = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(runchoice));
    if (strcmp("Run test case", choice) == 0)
	gtk_widget_show_all(GTK_WIDGET(testcasebox));
    else
	gtk_widget_hide(GTK_WIDGET(testcasebox));
}

gboolean on_asswindow_delete_event(GtkWidget *w, GdkEvent *e, gpointer data)
{
    gtk_widget_hide(w);
    return TRUE;
}

void on_close_button_press_event(void)
{
    gtk_main_quit();
}

/* helper functions */
static void *run(void *pl)
{
    char text[BUFSIZ];
    int tc = atoi(gtk_entry_get_text(testcase)); // Test case
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
    int tc = atoi(gtk_entry_get_text(testcase)); // Test case
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

void setUIvals(void)
{
    snprintf(UI.DOC, sizeof(UI.DOC), "%s", gtk_entry_get_text(DOC));
    snprintf(UI.DR, sizeof(UI.DR), "%s", gtk_entry_get_text(DR));
    snprintf(UI.agecorr, sizeof(UI.agecorr), "%s", gtk_entry_get_text(agecorr));
    snprintf(UI.infl, sizeof(UI.infl), "%s", gtk_entry_get_text(infl));
    snprintf(UI.TRM_PercDef, sizeof(UI.TRM_PercDef), "%s", gtk_entry_get_text(TRM_PercDef));
    snprintf(UI.DR113, sizeof(UI.DR113), "%s", gtk_entry_get_text(DR113));

    if (gtk_toggle_button_get_active(fixedSIradiobutton))
	snprintf(UI.SS, sizeof(UI.SS), "%s", gtk_entry_get_text(fixedSIentry));
    else 
    { 
	/* Text view is rather tedious to retrieve text from, this is why this seems so random */
	GtkTextBuffer *temp = gtk_text_view_get_buffer(SS);
	GtkTextIter begin, end;
	gtk_text_buffer_get_iter_at_offset(temp, &begin, (gint)0);
	gtk_text_buffer_get_iter_at_offset(temp, &end, (gint)-1);
	snprintf(UI.SS, sizeof(UI.SS), "%s", gtk_text_buffer_get_text(temp, &begin, &end, TRUE));
    }

    snprintf(UI.standard, sizeof(UI.standard), "%s",
	    gtk_combo_box_text_get_active_text(standard));
    snprintf(UI.assets, sizeof(UI.assets), "%s", 
	    gtk_combo_box_text_get_active_text(assets));
    snprintf(UI.paragraph, sizeof(UI.paragraph), "%s", 
	    gtk_combo_box_text_get_active_text(paragraph));
    snprintf(UI.PUCTUC, sizeof(UI.PUCTUC), "%s", 
	    gtk_combo_box_text_get_active_text(PUCTUC));
    snprintf(UI.cashflows, sizeof(UI.cashflows), "%s", 
	    gtk_combo_box_text_get_active_text(cashflows));
    snprintf(UI.evaluateDTH, sizeof(UI.evaluateDTH), "%s", 
	    gtk_combo_box_text_get_active_text(evaluateDTH));
}

unsigned short validateUI(void)
{
    char msg[BUFSIZ] = "The following invalid data was found:\n\n";
    char temp[BUFSIZ];
    int cntErr = 0;

    /* ----- Check DOC -----*/
    char *day, *month, *year;
    strcpy(temp, UI.DOC);

    day = strtok(temp, "/");
    month = strtok(NULL, "/");
    year = strtok(NULL, "");

    if (day == NULL || month == NULL || year == NULL)
    {
	setMsgErr(msg, "DOC", UI.DOC, dateErr);
	cntErr++;
    }
    else
    {
	Date *tempDate = newDate(0, atoi(year), atoi(month), atoi(day));
	if (tempDate == NULL)
	{
	    setMsgErr(msg, "DOC", UI.DOC, dateErr);
	    cntErr++;
	}
	free(tempDate);
    }

    /* ----- Check DR -----*/
    if (!isfloat(UI.DR))
    {
	setMsgErr(msg, "DR", UI.DR, floatErr);
	cntErr++;
    }

    /* ----- Check Age Correction -----*/
    if (!isint(UI.agecorr))
    {
	setMsgErr(msg, "Age Correction", UI.agecorr, agecorrErr);
	cntErr++;
    }

    /* ----- Check Inflation -----*/
    if (!isfloat(UI.infl))
    {
	setMsgErr(msg, "Inflation", UI.infl, floatErr);
	cntErr++;
    }

    /* ----- Check Termination percentage -----*/
    if (!isfloat(UI.TRM_PercDef))
    {
	setMsgErr(msg, "Termination % (usually 1)", UI.TRM_PercDef, floatErr);
	cntErr++;
    }

    /* ----- Check DR 113 -----*/
    if (!isfloat(UI.DR113))
    {
	setMsgErr(msg, "DR $113", UI.DR113, floatErr);
	cntErr++;
    }

    /* ----- Check Salary Increase -----*/
    if (gtk_toggle_button_get_active(fixedSIradiobutton))
    {
	if (!isfloat(UI.SS))
	{
	    setMsgErr(msg, "Salary Increase", UI.SS, floatErr);
	    cntErr++;
	}
    }

    printf("%s\n", msg);
    return 0;
}

void setMsgErr(char msg[], const char *input, const char UIs[], Err err)
{
    char temp[BUFSIZ];
    snprintf(temp, BUFSIZ, 
	    "%s%s: [%s], but should be of the form %s\n", msg, input, UIs, validMsg[err]);
    strcpy(msg, temp);
}
