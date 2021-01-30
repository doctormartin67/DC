#include <gtk/gtk.h>

void userinterface(int argc, char **argv);

// signal functions
static void destroy (GtkWidget *, gpointer);

// helper functions
GtkWidget *createwindow(char *title, int width);

static const char *labellist[] = {"DR:"};

void userinterface(int argc, char **argv) {
	int lengthlist = sizeof(labellist) / sizeof(labellist[0]); 
	GtkWidget *window, *button, *vbox, *assgrid, *assgridlabel, *entry;

	gtk_init (&argc, &argv);

	window = createwindow("DC Program", 10);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 50);

	button = gtk_button_new_with_label("Run");
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy),
			(gpointer) window);
	gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	assgrid = gtk_grid_new();	
	assgridlabel = gtk_label_new("Client Assumptions:");
	gtk_grid_attach(GTK_GRID(assgrid), assgridlabel, 0, 0, 2, 1);
	for (int i = 0; i < lengthlist; i++) {
		assgridlabel = gtk_label_new(labellist[i]);
		entry = gtk_entry_new();
		gtk_grid_attach(GTK_GRID(assgrid), assgridlabel, 0, i+1, 1, 1);	
		gtk_grid_attach(GTK_GRID(assgrid), entry, 1, i+1, 1, 1);	
	}
	gtk_grid_set_row_spacing (GTK_GRID(assgrid), 20);
	gtk_grid_set_column_spacing (GTK_GRID(assgrid), 20);

	gtk_box_pack_end(GTK_BOX(vbox), assgrid, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_widget_show_all (window);
	gtk_main ();
}

//***Start signal functions***
static void destroy (GtkWidget *window, gpointer data) {
	gtk_main_quit ();
}
//***End signal functions***

//***Start helper functions***
GtkWidget *createwindow(char *title, int width) {
	GtkWidget *window;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), title); 
	gtk_container_set_border_width(GTK_CONTAINER(window), width);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
	
	return window;
}
//***End helper functions***
