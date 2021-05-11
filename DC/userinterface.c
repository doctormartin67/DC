#include <gtk/gtk.h>

#define GLADEFILE "DCProgram.glade"

void userinterface(int argc, char **argv);

/* signal functions */
void on_fixedSIradiobutton_toggled(GtkRadioButton *);
void on_variableSIradiobutton_toggled(GtkRadioButton *);

/* Objects */
GtkBuilder *builder;
GtkWidget *window;
GtkRadioButton *fixedSIradiobutton;
GtkRadioButton *variableSIradiobutton;

void userinterface(int argc, char **argv) {
    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init (&argc, &argv);

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
void on_fixedSIradiobutton_toggled(GtkRadioButton *rb) {

}

void on_variableSIradiobutton_toggled(GtkRadioButton *rb) {

}
