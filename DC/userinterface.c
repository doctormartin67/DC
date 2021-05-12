#include <gtk/gtk.h>

#define GLADEFILE "DCProgram.glade"

void userinterface(int argc, char **argv);

/* signal functions */
void on_fixedSIradiobutton_toggled(GtkRadioButton *);
void on_variableSIradiobutton_toggled(GtkRadioButton *);

/* Objects */
static GtkBuilder *builder;
static GtkWidget *window;
static GtkWidget *fixedSIentry;
static GtkWidget *interpreterbutton;
static GtkRadioButton *fixedSIradiobutton;
static GtkRadioButton *variableSIradiobutton;

void userinterface(int argc, char **argv) {
    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init (&argc, &argv);

    builder = gtk_builder_new_from_file(GLADEFILE);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    fixedSIentry = GTK_WIDGET(gtk_builder_get_object(builder, "fixedSIentry"));
    interpreterbutton = GTK_WIDGET(gtk_builder_get_object(builder, "interpreterbutton"));

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_builder_connect_signals(builder, NULL);

    gtk_widget_show(window);

    /* Hand control over to the main loop. */
    /* It will continue to run until gtk_main_quit() is called or the application
       terminates. It sleeps and waits for signals to be emitted. */
    gtk_main();
}

/* signal functions */
void on_fixedSIradiobutton_toggled(GtkRadioButton *rb) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb))) {
	gtk_widget_set_sensitive(interpreterbutton, FALSE);
	gtk_widget_set_sensitive(fixedSIentry, TRUE);
    }
    else {
	gtk_widget_set_sensitive(interpreterbutton, TRUE);
	gtk_widget_set_sensitive(fixedSIentry, FALSE);
    }
}

void on_variableSIradiobutton_toggled(GtkRadioButton *rb) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb))) {
	gtk_widget_set_sensitive(fixedSIentry, FALSE);
	gtk_widget_set_sensitive(interpreterbutton, TRUE);
    }
    else {
	gtk_widget_set_sensitive(fixedSIentry, TRUE);
	gtk_widget_set_sensitive(interpreterbutton, FALSE);
    }
}
