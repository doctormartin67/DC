#include <gtk/gtk.h>
#include <threads.h>
#include "DCProgram.h"

#define GLADEFILE "DCProgram.glade"

typedef struct {
    GtkButton *b; /* run button */
    GtkWidget *pl; /* progresslabel */
} Run;

static DataSet *ds;

void runmember(CurrentMember *cm);
static GtkWidget *runchoice; /* used for combo box text to choose which run option */

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *, GtkWidget *);
void on_startstopbutton_clicked(GtkButton *, GtkWidget *);

/* helper functions */
static int runth(void *);

void userinterface(DataSet *pds) {
    GtkBuilder *builder;
    GtkWidget *window;

    ds = pds;

    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init(NULL, NULL);

    builder = gtk_builder_new_from_file(GLADEFILE);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    runchoice = GTK_WIDGET(gtk_builder_get_object(builder, "runchoice"));

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

void on_startstopbutton_clicked(GtkButton *b, GtkWidget *pl) {
    static thrd_t *th = NULL; 
    if (th == NULL) {
	char *choice = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(runchoice));
	if (strcmp("Run one run", choice) == 0) {
	    th = malloc(sizeof(thrd_t));
	    currrun = runNewRF; // This needs updating when I start with reconciliation runs!!
	    (void)thrd_create(th, runth, (void *)pl);
	    (void)thrd_detach(*th);
	}
    }
    else {
	/* this still needs improved when I've read about threads in the new book!!! */
	gtk_label_set_text(GTK_LABEL(pl), ""); 
    }
}

static int runth(void *pl) {
    char text[128];
    CurrentMember *cm = ds->cm;

    for (int i = 0; i < ds->membercnt; i++) {
	setassumptions(cm + i);
	runmember(cm + i);
	snprintf(text, sizeof(text), 
		"Progress: member %d out of %d members complete", i + 1, ds->membercnt);
	gtk_label_set_text(GTK_LABEL(pl), text); 
    }

    // create excel file to print results
    int tc = 5; // Test case
    tc -= 1; // Index is one less than given test case
    printresults(ds, tc);
    return 0;
}
