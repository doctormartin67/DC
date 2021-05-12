#include <gtk/gtk.h>
#include "DCProgram.h"

#define GLADEFILE "DCProgram.glade"

typedef struct {
    GtkWidget *pb;
    double total;
    double timer;
} Progressbar;

static DataSet *ds;
static Progressbar pb;

void runmember(CurrentMember *cm);
void runonerun(DataSet *ds);
static gboolean progresstimer(gpointer);

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *, GtkWidget *);
void on_runonerunbutton_clicked(GtkButton *, GtkWidget *);

void userinterface(DataSet *pds) {
    GtkBuilder *builder;
    GtkWidget *window;

    ds = pds;

    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init(NULL, NULL);

    builder = gtk_builder_new_from_file(GLADEFILE);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_builder_connect_signals(builder, (void *)NULL);

    pb.pb = GTK_WIDGET(gtk_builder_get_object(builder, "progressbar"));
    pb.total = (double)ds->membercnt;
    pb.timer = 0.0;

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

void on_runonerunbutton_clicked(GtkButton *b, GtkWidget *null) {

    // Here the loop of all affiliates will start.
    currrun = runNewRF; // This needs updating when I start with reconciliation runs!!
    CurrentMember *cm = ds->cm;
    g_timeout_add(100, progresstimer, NULL);

    for (int i = 0; i < ds->membercnt; i++) {
	setassumptions(cm + i); // This defines the assumptions
	runmember(cm + i);
	pb.timer = (double)i + 1;
    }

    // create excel file to print results
    int tc = 2; // Test case
    tc -= 1; // Index is one less than given test case
    printresults(ds, tc);

}

static gboolean progresstimer(gpointer null) {
    static int count = 0;
    printf("count: %d\n", count++);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb.pb), pb.timer/pb.total);
    return pb.timer != pb.total;
}
