#include <assert.h>
#include "userinterface.h"

/*
 * This equals the current index of the interpreter that will be set by the
 * open interpreter window once it closes
 */
static unsigned current_interpreter;

static void set_interpreter_text(unsigned intprtr);

void on_close_button_press_event(void)
{
	gtk_main_quit();
}

/*
 * Open .dc file that will fill in the current user interface
 */
void on_openDC_activate(GtkMenuItem *m)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	char *filename = 0;
	gint res = 0;
	GtkDialog *dialog = 0;
	GtkFileChooser *chooser = 0;

	dialog = GTK_DIALOG(widgets[OPENDCFILE]);
	gtk_widget_show(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog); 

	if (res == GTK_RESPONSE_ACCEPT) {
		chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);
		printf("selected file [%s] to open\n", filename);

		readHashtable(filename, ht);
		update_user_interface(ht);

		g_free(filename);
		filename = 0;
	} else {
		printf("Cancelled [%s]\n", gtk_menu_item_get_label(m));
	}

	gtk_widget_hide(GTK_WIDGET(dialog));
	printHashtable(ht);
}

void on_saveDC_activate(GtkMenuItem *m)
{
	printf("[%s] activated\n", gtk_menu_item_get_label(m));
}

/*
 * save .dc file as given name with the current user input values
 */
void on_saveasDC_activate(GtkMenuItem *m)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	char tmp[BUFSIZ];
	char *filename = 0;
	char *p = 0;
	GtkDialog *dialog = 0;
	GtkFileChooser *chooser = 0;
	gint res = 0;

	set_user_input(ht);
	dialog = GTK_DIALOG(widgets[SAVEASDCFILE]);
	gtk_widget_show(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog); 

	if (res == GTK_RESPONSE_ACCEPT) {
		chooser = GTK_FILE_CHOOSER(dialog);
		p = filename = gtk_file_chooser_get_filename(chooser);

		if (0 == (p = strstr(p, ".dc"))) {
			snprintf(tmp, sizeof(tmp), "%s.dc", filename);
		} else {
			while (*p != '\0') p++;
			p -= 3;
			if (strcmp(p, ".dc") == 0)
				snprintf(tmp, sizeof(tmp), "%s", filename);
			else
				snprintf(tmp, sizeof(tmp), "%s.dc", filename);
		}
		printf("selected file [%s] to save\n", tmp);

		writeHashtable(tmp, ht);

		g_free(filename);
		p = filename = 0;
	} else {
		printf("Cancelled [%s]\n", gtk_menu_item_get_label(m));
	}

	gtk_widget_hide(GTK_WIDGET(dialog));
}

/*
 * sets the file name of the user input to the chosen excel file
 */
void on_LYfilechooserbutton_file_set(GtkFileChooserButton *b, gpointer p)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	if (0 != p) printf("unused pointer [%p]\n", p);
	printf("dialog [%s] closed\n", gtk_file_chooser_button_get_title(b));

	GtkDialog *dialog = 0;
	char *filename = 0;
	char tmp[BUFSIZ];

	dialog = GTK_DIALOG(widgets[OPENEXCELFILE]);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	filename = gtk_file_chooser_get_filename(chooser);

	ht_set("fname", filename, ht);
	snprintf(tmp, sizeof(tmp), "File set to run:\n%s", ht_get("fname", ht));

	printf("Excel to run: [%s]\n", ht_get("fname", ht));
	gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), tmp); 
	g_free(filename);
}

/*
 * when interpreter window is closed, we don't want it to be deleted, but just
 * hidden
 * the function will set the current interpreter to the text inside the window
 * the current interpreter will be set depending on which button was pressed
 * to open the interpreter
 */
gboolean on_interpreterwindow_delete_event(GtkWidget *w, GdkEvent *e,
		gpointer p)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	if (0 != p) printf("unused pointer [%p]\n", p);
	printf("GdkEventType [%d]\n", gdk_event_get_event_type(e));

	GtkTextBuffer *temp = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(widgets[INTERPRETERTEXT]));
	gchar *s = 0;
	GtkTextIter begin, end;
	gtk_text_buffer_get_iter_at_offset(temp, &begin, 0);
	gtk_text_buffer_get_iter_at_offset(temp, &end, -1);
	s = gtk_text_buffer_get_text(temp, &begin, &end, TRUE);

	assert(current_interpreter < UI_AMOUNT) ;

	ht_set(ui_interpreter_variables[current_interpreter].key, s, ht);
	g_free(s);

	gtk_widget_hide(w);
	return TRUE;
}

/*
 * opens the interpreter and sets current_interpreter to the salary scale
 * array address
 */
void on_SS_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_SS);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the turnover
 * array address
 */
void on_turnover_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_TURNOVER);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the retx
 * array address
 */
void on_retx_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_RETX);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the NRA
 * array address
 */
void on_NRA_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_NRA);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_admincost_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_ADMINCOST);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_costRES_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_COSTRES);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_costKO_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_COSTKO);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_WD_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_WD);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_prepost_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_PREPOST);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_term_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_TERM);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_ltINS_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_LTINS);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_ltTERM_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_LTTERM);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_contrA_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_CONTRA);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_contrC_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(UI_CONTRC);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * sets the interpreter text to the given interpreter and sets the title of
 * the interpreter window to s
 * also updates the current_interpreter to intprtr
 */
static void set_interpreter_text(unsigned intprtr)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	const char *s = ui_interpreter_variables[intprtr].key;
	const char *t = ht_get(s, ht);
	GtkTextBuffer *temp = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(widgets[INTERPRETERTEXT]));

	if (t) gtk_text_buffer_set_text(temp, t, -1);
	current_interpreter = intprtr;
	assert(current_interpreter < UI_AMOUNT);
	gtk_window_set_title(GTK_WINDOW(widgets[INTERPRETERWINDOW]), s);
}
