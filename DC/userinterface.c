#include <gtk/gtk.h>
#include "DCProgram.h"

#define GLADEFILE "DCProgram.glade"

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *, GtkWidget *);

void userinterface(DataSet *ds) {
    GtkBuilder *builder;
    GtkWidget *window;

    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init(NULL, NULL);

    builder = gtk_builder_new_from_file(GLADEFILE);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_builder_connect_signals(builder, NULL);

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
