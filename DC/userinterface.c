#include <pthread.h>
#include "DCProgram.h"

#define GLADEFILE "DCProgram.glade"

static unsigned short running;
static DataSet *ds;
static pthread_t thrun;

void runmember(CurrentMember *cm);
static GtkWidget *runchoice; /* used for combo box text to choose which run option */
static GtkWidget *testcasebox; /* used to hide and show box with test case input */
static GtkEntry *testcase; /* this is the actual test case to run */

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *, GtkWidget *);
void on_startstopbutton_clicked(GtkButton *, GtkWidget *);
void on_SIinterpreterbutton_clicked(GtkButton *b, gpointer *p);
void on_runchoice_changed(GtkComboBox *cb, gpointer *p);
gboolean on_asswindow_delete_event(GtkWidget *, GdkEvent *, gpointer);

/* helper functions */
static void *run(void *);
static void *runtc(void *pl);

void userinterface(DataSet *pds) {
    GtkBuilder *builder;
    GtkWidget *window, *asswindow;

    ds = pds;

    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init(NULL, NULL);

    builder = gtk_builder_new_from_file(GLADEFILE);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    runchoice = GTK_WIDGET(gtk_builder_get_object(builder, "runchoice"));
    testcasebox = GTK_WIDGET(gtk_builder_get_object(builder, "testcasebox"));
    testcase = GTK_ENTRY(gtk_builder_get_object(builder, "testcase"));

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
	running = TRUE;
	int s = 0; /* used for error printing */
	char *choice = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(runchoice));
	if (strcmp("Run one run", choice) == 0)
	{
	    currrun = runNewRF; // This needs updating when I start with reconciliation runs!!
	    s = pthread_create(&thrun, NULL, run, (void *)pl);
	    if (s != 0)
		errExitEN(s, "[%s] unable to create thread\n", __func__);

	    s = pthread_detach(thrun);
	    if (s != 0)
		errExitEN(s, "[%s] unable to detach thread\n", __func__);
	}
	else if (strcmp("Run test case", choice) == 0)
	{
	    currrun = runNewRF; // This needs updating when I start with reconciliation runs!!
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
