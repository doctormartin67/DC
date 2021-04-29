#include <gtk/gtk.h>

#define PATH "/home/doctormartin67/Projects/library/" // Needs updating!!

enum {WIDTH = 3, BORDER = 10}; // This is the width of the grids inside the notebook boxes

/* This will be the default size of the window on start up. after that the user can adjust the 
   window how he pleases. */
enum {ROWSIZE = 600, COLUMNSIZE = 500}; 

/* the following enum elements are self created response identifiers for the dialog used in
   the assumptions like salary increase and turnover that have specific rule */
enum {RESPONSE_AGE, RESPONSE_NO_REGLEMENT, RESPONSE_CATEGORY};
enum {MAXCOMMENT = 128}; // Maximum characters in a line of text, usually a comment

typedef struct {
    GtkWidget *grid;
    unsigned short index;
} Gridinput;

void userinterface(int argc, char **argv);

// signal functions
static void destroy (GtkWidget *, gpointer);
static gboolean delete_event(GtkWidget *window, GdkEvent *event, gpointer data);
static void fixed_button_toggled(GtkToggleButton *toggle, GtkWidget *other_toggle);
static void rule_button_toggled(GtkToggleButton *toggle, GtkWidget *other_toggle);
static void file_changed(GtkFileChooser *chooser, GtkLabel *label);
static void addremoverule(GtkButton *button, gpointer data);

// helper functions
GtkWidget *createwindow(char *title, int width, int col, int row);
GtkWidget *createrunbutton(GtkWidget *window, GtkWidget *windowbox);
void createassgrid(GtkWidget *notebookbox);
void addassnotebook(char *label, GtkWidget *notebook);
GtkWidget **addEntryToGrid(GtkWidget *grid, char *name, int x, int y, 
	unsigned short entriescnt, gboolean isbold);

/* a rule grid will consist of a grid with a label describing the rule, the two toggle buttons
   describing whether the assumption is a fixed amount of depends on a rule */
GtkWidget *createrulegrid(char *ass, char *comment);


void userinterface(int argc, char **argv) {
    GtkWidget *window;
    GtkWidget *scrolledwindow;
    GtkWidget *windowbox; // This will contain the notebook and Run button
    GtkWidget *notebook;

    /* Initialize GTK+ and all of its supporting libraries. */
    gtk_init (&argc, &argv);

    /* the second amount is the minimal border size. the third and fourth parameters are the
       column and row width in pixels. These will determine the minimal size of the window. */
    /* I have chosen -1 as to let Gtk+ choose the size because each pc might look different */
    window = createwindow("DC Program", BORDER, -1, -1);

    /* This might not always be visible, depends on your window manager */
    gtk_window_set_icon_from_file(GTK_WINDOW(window), PATH "Aonlogo.png", NULL);

    /* first parameter determines orientation of widgets in box. second parameter is the spacing
       between each item in box. In this case, it's the distance between the run button and the
       items above. */
    windowbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    (void)createrunbutton(window, windowbox);

    /* This will hold the window box so that everything will be scrollable, just in case there
       are many assumptions that would make the window go off screen */
    scrolledwindow = gtk_scrolled_window_new(NULL, NULL);

    notebook = gtk_notebook_new();
    addassnotebook("Assumptions Last Year", notebook);
    addassnotebook("Assumptions This Year", notebook);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE); /* make sure you can scroll 
								  through tabs if there are 
								  too many */

    gtk_container_add(GTK_CONTAINER(windowbox), notebook);
    gtk_container_add(GTK_CONTAINER(scrolledwindow), windowbox);
    gtk_container_add(GTK_CONTAINER(window), scrolledwindow);

    /* This will show the window and all it's children inside the window. Note that it does
       not show the parent of the window (if any) and so it will be queued until the parent is shown */
    gtk_widget_show_all(window);

    /* Hand control over to the main loop. */
    /* It will continue to run until gtk_main_quit() is called or the application
       terminates. It sleeps and waits for signals to be emitted. */
    gtk_main();
}

//***Start signal functions***
/* signals are inherited from parents */
static void destroy(GtkWidget *window, gpointer data) {
    gtk_main_quit();
}

/* Return FALSE to destroy the widget (gtk_widget_destroy() is called in this case). 
   By returning TRUE, you can cancel a delete-event. This can be used to confirm quitting
   the application. a GdkEvent is first recognised by the underlying GdkWindow of the widget 
   and then emitted as a signal of the window. In other words, they are initially emitted
   by the window manager (X Window System for example) and then sent to the application.
   (The X button to close the window is an example of this) */
/* GdkEvent is a union of the GdkEventType enumeration and all the available event structures. 
   for example, you could check delete event by using event->type == GDK_DELETE */
static gboolean delete_event(GtkWidget *window, GdkEvent *event, gpointer data) {
    return FALSE;
}

/* Some assumptions can be fixed or dependant or other things like category, plan rules, age , ...
   This function sets the fixed amount option on or off and deletes the text in the entry when 
   it is turned off. */
static void fixed_button_toggled(GtkToggleButton *toggle, GtkWidget *entry) {
    if (gtk_toggle_button_get_active (toggle))
	gtk_widget_set_sensitive(entry, TRUE);
    else {
	gtk_editable_delete_text(GTK_EDITABLE(entry), 0, -1);
	gtk_widget_set_sensitive(entry, FALSE);
    }
}

/* This will create a dialog window where the user needs to input the rules for the assumption
   that will be based on a given criteria. I called the second argument "null" because it is
   not used */

static void rule_button_toggled(GtkToggleButton *toggle, GtkWidget *button) {
    if (gtk_toggle_button_get_active (toggle))
	gtk_widget_set_sensitive(button, TRUE);
    else
	gtk_widget_set_sensitive(button, FALSE);
}

/* When a file is selected, display the full path in the GtkLabel widget. */
static void file_changed (GtkFileChooser *chooser, GtkLabel *label) {
    gchar *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    gtk_label_set_text (label, file);
}

/* Create a new GtkDialog that can be used to add or remove a rule from the assumption. */
static void addremoverule(GtkButton *button, gpointer data) {
    GtkWidget *dialog, *label, *contentarea, **entry;
    Gridinput *pdata = (Gridinput *)data;
    unsigned short response;
    char text[MAXCOMMENT];

    /* The dialog that will be created will have three options to choose from:
       add/remove age, add/remove plan rule, add/remove category */
    dialog = gtk_dialog_new_with_buttons ("Add or remove a rule", NULL, 
	    GTK_DIALOG_MODAL,
	    "AGE", RESPONSE_AGE, 
	    "NO REGLEMENT",RESPONSE_NO_REGLEMENT, 
	    "CATEGORY", RESPONSE_CATEGORY,
	    NULL);
    label = gtk_label_new("Pick a rule to add or remove:");
    contentarea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(contentarea), label);
    gtk_container_set_border_width(GTK_CONTAINER(contentarea), BORDER);

    gtk_widget_show_all(dialog);

    /* Create the dialog as modal and destroy it when a button is clicked. */
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    switch (response) {
	case RESPONSE_AGE:
	    strcpy(text, "Age: (f.e. 40 0.05 means assumption is 0.05 for Age<40)");
	    break;
	case RESPONSE_NO_REGLEMENT:
	    strcpy(text, "Reglement:");
	    break;
	case RESPONSE_CATEGORY:
	    strcpy(text, "Category:");
	    break;
	default:
	    strcpy(text, "TBD");
	    break;
    }
    entry = addEntryToGrid(pdata->grid, text, 0, pdata->index++, 2, FALSE);
    gtk_widget_show_all(pdata->grid);
    gtk_widget_destroy(dialog);
}
//***End signal functions***

//***Start helper functions***
GtkWidget *createwindow(char *title, int width, int col, int row) {
    GtkWidget *window;

    /* Create a new window and give it a title */
    /* The default width and height is 200 pixels and is chosen so because if they were 0
       pixels the window would not be able to be resized. the title bar and window border
       are included in this size. 
       GTK_WINDOW_TOPLEVEL means that the window manager has ultimate control over the 
       features. https://developer.gnome.org/gtk3/stable/GtkWindow.html#GtkWindowType */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    /* to change the default window size, use the following:
       Confusingly, the first number (200) is the column width in pixels and the second number (100)
       is the row width. the two numbers are the minimum size that the window can be. using -1
       for either col or row will force the widget to take its natural size. this can be useful
       if you only want to specify either col or row */
    gtk_widget_set_size_request(window, col, row);

    /* because a scrollable window will be added to this window, the default size needs to be set
       just so that when the program starts, it's not really tiny. The user can still adjust 
       the window how he pleases, this is just for initial start up */
    gtk_window_set_default_size(GTK_WINDOW(window), COLUMNSIZE, ROWSIZE);

    /* https://developer.gnome.org/gtk3/stable/GtkWindow.html#gtk-window-set-title */
    gtk_window_set_title(GTK_WINDOW(window), title); 

    /* width will be the amount of pixels of the minimal size of the border around the 
       window. In other words, the window will have a border of atleast the width. */
    gtk_container_set_border_width(GTK_CONTAINER(window), width);

    /* This attaches the destroy signal to the window, which is emitted when 
       gtk_widget_destroy() is called on the widget or when FALSE is returned from a 
       delete_event() callback function.  the delete_event() function doesn't automatically
       exit gtk, you have to use gtk_main_quit() to do this. Returning TRUE from a delete_event
       can be useful to ask "are you sure" from the user and thus cancel the delete. */
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);

    return window;
}

GtkWidget *createrunbutton(GtkWidget *window, GtkWidget *windowbox) {
    GtkWidget *button;

    button = gtk_button_new_with_label("Run");

    /* Relief is a type of 3-D border that distinguishes the button from other widgets */
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);

    g_signal_connect_swapped (G_OBJECT (button), "clicked",
	    G_CALLBACK (gtk_widget_destroy),
	    (gpointer) window);

    /* https://developer.gnome.org/gtk3/stable/GtkBox.html#gtk-box-pack-end */
    /* Expanding is set to TRUE, which will automatically provide the cell with the extra 
       space allocated to the box. This space is distributed evenly to all of the cells that
       request it. The fill property is also set to TRUE, which means the widget will expand 
       into all of the extra space provided instead of filling it with padding. Lastly, 
       the amount of padding placed between the cell and its neighbors is set to zero pixels. */
    gtk_box_pack_end(GTK_BOX(windowbox), button, FALSE, FALSE, 0);

    return button;
}

/* each notebook page has a verticle box where a grid gets added to pick the assumptions. */
void createassgrid(GtkWidget *notebookbox) {
    GtkWidget *assgrid, *assgridlabel, *expander, *rulegrid, *file, *filelabel;
    GtkFileFilter *filter;
    char *ass; // This will be the name of the assumption for which we need to create a grid
    char text[MAXCOMMENT]; // used to set some text to bold using markup
    int index = 0;

    assgrid = gtk_grid_new();	
    assgridlabel = gtk_label_new(NULL);
    snprintf(text, sizeof(text), "%s%s%s", "<big>", "Client Assumptions", "</big>");
    gtk_label_set_markup(GTK_LABEL(assgridlabel), text);

    gtk_label_set_selectable(GTK_LABEL(assgridlabel), TRUE);

    /***********************/
    /* add file chooser to notebook */
    filelabel = gtk_label_new ("");
    file = gtk_file_chooser_button_new("Select input data to run", GTK_FILE_CHOOSER_ACTION_OPEN);
    g_signal_connect(G_OBJECT(file), "selection_changed", 
	    G_CALLBACK(file_changed), (gpointer)filelabel);
    /* Provide a filter to show only excel files. */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, "Excel file");
    gtk_file_filter_add_pattern (filter, "*.xls*");
    /* Add the filter to the file chooser button that selects files. */
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file), filter);
    gtk_grid_attach(GTK_GRID(assgrid), file, 0, index++, WIDTH, 1);
    /***********************/

    gtk_grid_attach(GTK_GRID(assgrid), assgridlabel, 0, index++, WIDTH, 1);

    /* Fixed entries in the grid, TRUE means they will be bold */ 
    (void)addEntryToGrid(assgrid, "DOC: ", 1, index++, 1, TRUE);
    (void)addEntryToGrid(assgrid, "DR: ", 1, index++, 1, TRUE);
    (void)addEntryToGrid(assgrid, "Age Correction: ", 1, index++, 1, TRUE);

    /* Salary increase is an assumption that might be based on multiple factors, such as category, 
       plan rules, age, ... */
    ass = "Salary increase";
    rulegrid = createrulegrid(ass, "This section describes the merit on top of inflation");

    /* Set this text to bold, uses Pango markup */
    snprintf(text, sizeof(text), "%s%s%s", "<b>", ass, "</b>");
    expander = gtk_expander_new_with_mnemonic(text);
    gtk_expander_set_use_markup(GTK_EXPANDER(expander), TRUE);
    gtk_container_add (GTK_CONTAINER(expander), rulegrid);
    gtk_grid_attach(GTK_GRID(assgrid), expander, 1, index++, 3, 1);	

    /* Fixed entries in the grid, TRUE means they will be bold */ 
    (void)addEntryToGrid(assgrid, "Inflation: ", 1, index++, 1, TRUE);

    /* Turnover is an assumption that might be based on multiple factors, such as category, 
       plan rules, age, ... */
    ass = "Turnover";
    rulegrid = createrulegrid(ass, "This section describes the turnover rate");
    snprintf(text, sizeof(text), "%s%s%s", "<b>", ass, "</b>");
    expander = gtk_expander_new_with_mnemonic (text);
    gtk_expander_set_use_markup(GTK_EXPANDER(expander), TRUE);
    gtk_container_add (GTK_CONTAINER(expander), rulegrid);
    gtk_grid_attach(GTK_GRID(assgrid), expander, 1, index++, 3, 1);	

    gtk_grid_set_row_spacing (GTK_GRID(assgrid), 10);
    gtk_grid_set_column_spacing (GTK_GRID(assgrid), 10);

    gtk_box_pack_end(GTK_BOX(notebookbox), assgrid, FALSE, FALSE, 10);
}

/* add a page to the notebook. there will only be two pages, one with last year assumptions and 
   one with this year assumptions. */
void addassnotebook(char *label, GtkWidget *notebook) {
    GtkWidget *notebookbox, *asslabel;
    notebookbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    createassgrid(notebookbox);
    gtk_container_set_border_width(GTK_CONTAINER(notebookbox), BORDER);

    asslabel = gtk_label_new(label);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), notebookbox, asslabel);
}

/* the assumptions grid consists of a label and an entry. This function will add one of these
   pairs to the grid */
GtkWidget **addEntryToGrid(GtkWidget *grid, char *name, int x, int y, unsigned short entriescnt, 
	gboolean isbold) {
    char text[MAXCOMMENT];
    GtkWidget *label, **entry;
    entry = (GtkWidget **)calloc(entriescnt, sizeof(GtkWidget *));

    if (isbold) {
	/* set label bold */
	label = gtk_label_new(NULL);
	snprintf(text, sizeof(text), "%s%s%s", "<b>", name, "</b>");
	gtk_label_set_markup(GTK_LABEL(label), text);
    }
    else
	label = gtk_label_new(name);

    /* set selectable means that the user can interact with the label (f.e. selecting and
       copying the text). */
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);

    for (int i = 0; i < entriescnt; i++)
    entry[i] = gtk_entry_new();

    /* https://developer.gnome.org/gtk3/unstable/GtkGrid.html#gtk-grid-attach */
    gtk_grid_attach(GTK_GRID(grid), label, x, y, 1, 1);	
    for (int i = 0; i < entriescnt; i++)
	gtk_grid_attach(GTK_GRID(grid), entry[i], x+i+1, y, 1, 1);	
    return entry;
}

GtkWidget *createrulegrid(char *ass, char *comment) {
    GtkWidget *rulegrid, *rulegridlabel, *fixedbutton, *rulebutton, **fixedentry, *addrulebutton;
    char text[MAXCOMMENT];
    unsigned index = 0;
    Gridinput *data = (Gridinput *)malloc(sizeof(Gridinput));

    rulegrid = gtk_grid_new();	
    rulegridlabel = gtk_label_new(NULL);
    snprintf(text, sizeof(text), "%s%s%s", "<i>", comment, "</i>");
    gtk_label_set_markup(GTK_LABEL(rulegridlabel), text);

    snprintf(text, sizeof(text), "%s%s", ass, " is a fixed amount.");
    fixedbutton = gtk_radio_button_new_with_mnemonic(NULL, text);
    snprintf(text, sizeof(text), "%s%s", ass, " is based on multiple rules.");
    rulebutton = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(fixedbutton), text);

    addrulebutton = gtk_button_new_with_mnemonic("add/remove rule");

    gtk_label_set_selectable(GTK_LABEL(rulegridlabel), TRUE);

    gtk_grid_attach(GTK_GRID(rulegrid), rulegridlabel, 0, index++, WIDTH, 1);
    gtk_grid_attach(GTK_GRID(rulegrid), fixedbutton, 0, index++, 1, 1);
    fixedentry = addEntryToGrid(rulegrid, "Fixed amount:", 0, index++, 1, FALSE);
    gtk_grid_attach(GTK_GRID(rulegrid), rulebutton, 0, index++, 1, 1);
    gtk_grid_attach(GTK_GRID(rulegrid), addrulebutton, 0, index++, 1, 1);

    /* when the button addremoverule is pressed, we need to add a rule to the grid, this is done
       by using data as a parameter that has the grid and the index where the rule should be 
       added */
    data->grid = rulegrid;
    data->index = index;

    /* These two signals toggle the options on and off */
    g_signal_connect(G_OBJECT(fixedbutton), "toggled",
	    G_CALLBACK(fixed_button_toggled),
	    (gpointer)*fixedentry);
    g_signal_connect(G_OBJECT(rulebutton), "toggled",
	    G_CALLBACK(rule_button_toggled), addrulebutton);

    /* This signal is to activate the dialog for the rule button to add or remove a rule */
    g_signal_connect(G_OBJECT(addrulebutton), "clicked",
	    G_CALLBACK(addremoverule), (gpointer)data);

    /* the rule button should start with not being able to be clicked until the radio button 
       for this option is on */
    gtk_widget_set_sensitive(addrulebutton, FALSE);

    return rulegrid;
}
//***End helper functions***
