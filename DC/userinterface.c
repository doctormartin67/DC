#include <gtk/gtk.h>

void userinterface(int argc, char **argv);

// signal functions
static void destroy (GtkWidget *, gpointer);

// helper functions
GtkWidget *createwindow(char *title, int width);
GtkWidget *createrunbutton(GtkWidget *window, GtkWidget *windowbox);
void createassgrid(GtkWidget *notebookbox);
void addassnotebook(char *label, GtkWidget *notebook);

static const char *asslist[] = {"DOC:", "DR:", "Age Correction", 
	"Salary Increase", "Inflation"};

void userinterface(int argc, char **argv) {
	GtkWidget *window;
	GtkWidget *windowbox; // This will contain the notebook and Run button
	GtkWidget *notebook;
	GtkWidget *button;

	gtk_init (&argc, &argv);

	window = createwindow("DC Program", 0);

	windowbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

	button = createrunbutton(window, windowbox);

	notebook = gtk_notebook_new();
	addassnotebook("Assumptions Last Year", notebook);
	addassnotebook("Assumptions This Year", notebook);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE); /* make sure you can scroll 
								      through tabs if there are 
								      too many */

	gtk_container_add (GTK_CONTAINER (windowbox), notebook);
	gtk_container_add (GTK_CONTAINER (window), windowbox);
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

GtkWidget *createrunbutton(GtkWidget *window, GtkWidget *windowbox) {
	GdkWindow *Gdkwindow;
	GdkDisplay *display;
	GdkCursor *cursor;
	GtkWidget *button;

	button = gtk_button_new_with_label("Run");
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy),
			(gpointer) window);
	gtk_box_pack_end(GTK_BOX(windowbox), button, FALSE, FALSE, 0);

	return button;
}

void createassgrid(GtkWidget *notebookbox) {
	int lengthlist = sizeof(asslist) / sizeof(asslist[0]); 
	GtkWidget *assgrid, *assgridlabel, *entry;
	
	assgrid = gtk_grid_new();	
	assgridlabel = gtk_label_new("Client Assumptions:");
	gtk_grid_attach(GTK_GRID(assgrid), assgridlabel, 0, 0, 3, 1);
	for (int i = 0; i < lengthlist; i++) {
		assgridlabel = gtk_label_new(asslist[i]);
		entry = gtk_entry_new();
		gtk_grid_attach(GTK_GRID(assgrid), assgridlabel, 1, i+1, 2, 1);	
		gtk_grid_attach(GTK_GRID(assgrid), entry, 3, i+1, 1, 1);	
	}
	gtk_grid_set_row_spacing (GTK_GRID(assgrid), 10);
	gtk_grid_set_column_spacing (GTK_GRID(assgrid), 10);

	gtk_box_pack_end(GTK_BOX(notebookbox), assgrid, FALSE, FALSE, 10);
}

void addassnotebook(char *label, GtkWidget *notebook) {
	GtkWidget *notebookbox, *asslabel;
	notebookbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	createassgrid(notebookbox);

	asslabel = gtk_label_new(label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), notebookbox, asslabel);
}
//***End helper functions***
