#include "userinterface.h"

/*
 * This points at the current text that will be set by the open
 * interpreter window once it closes
 */
static char *current_interpreter;

static void set_interpreter_text(char *intprtr, const char *s);

void on_close_button_press_event(void)
{
	gtk_main_quit();
}

/*
 * Open .dc file that will fill in the current user interface
 */
void on_openDC_activate(GtkMenuItem *m)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	FILE *fp = 0;
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

		fp = fopen(filename, "rb");
		if (0 == fp) die("Unable to open file [%s]", filename);

		if (fread(ui, sizeof(*ui), 1, fp) != 1)
			die("[%s] not a correct .dc file\n", filename);

		update_user_interface(ui);

		if (fclose(fp) == EOF) 
			die("unable to close file [%s]", filename);

		g_free(filename);
		filename = 0;
	} else {
		printf("Cancelled [%s]\n", gtk_menu_item_get_label(m));
	}

	gtk_widget_hide(GTK_WIDGET(dialog));
	print_user_input(ui);
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
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	FILE *fp = 0;
	char tmp[BUFSIZ];
	char *filename = 0;
	char *p = 0;
	GtkDialog *dialog = 0;
	GtkFileChooser *chooser = 0;
	gint res = 0;

	set_user_input(ui);
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

		fp = fopen(tmp, "wb");
		if (0 == fp) die("Unable to open file [%s]\n", tmp);

		if (fwrite(ui, sizeof(*ui), 1, fp) != 1)
			die("unable to write to file [%s]", tmp);
		if (fclose(fp) == EOF) 
			die("unable to close file [%s]", filename);

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
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	if (0 != p) printf("unused pointer [%p]\n", p);
	printf("dialog [%s] closed\n", gtk_file_chooser_button_get_title(b));

	GtkDialog *dialog = 0;
	char *filename = 0;
	char tmp[BUFSIZ];

	dialog = GTK_DIALOG(widgets[OPENEXCELFILE]);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	filename = gtk_file_chooser_get_filename(chooser);

	snprintf(ui->fname, sizeof(ui->fname), "%s", filename);
	snprintf(tmp, sizeof(tmp), "File set to run:\n%s", ui->fname);

	printf("Excel to run: [%s]\n", ui->fname);
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
	if (0 != p) printf("unused pointer [%p]\n", p);
	printf("GdkEventType [%d]\n", gdk_event_get_event_type(e));

	GtkTextBuffer *temp = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(widgets[INTERPRETERTEXT]));
	gchar *s = 0;
	GtkTextIter begin, end;
	gtk_text_buffer_get_iter_at_offset(temp, &begin, 0);
	gtk_text_buffer_get_iter_at_offset(temp, &end, -1);
	s = gtk_text_buffer_get_text(temp, &begin, &end, TRUE);

	if (0 == current_interpreter) 
		die("current_interpreter was not set, this is impossible");
	snprintf(current_interpreter, INTERPRETERTEXT_SIZE, "%s", s);
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
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_SS], ui_var_names[UI_SS]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the turnover
 * array address
 */
void on_turnover_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_TURNOVER], ui_var_names[UI_TURNOVER]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the retx
 * array address
 */
void on_retx_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_RETX], ui_var_names[UI_RETX]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the NRA
 * array address
 */
void on_NRA_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_NRA], ui_var_names[UI_NRA]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_admincost_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_ADMINCOST],
			ui_var_names[UI_ADMINCOST]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_costRES_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_COSTRES], ui_var_names[UI_COSTRES]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_costKO_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_COSTKO], ui_var_names[UI_COSTKO]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_WD_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_WD], ui_var_names[UI_WD]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_prepost_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_PREPOST], ui_var_names[UI_PREPOST]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_term_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(ui->var[UI_TERM], ui_var_names[UI_TERM]);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * sets the interpreter text to the given interpreter and sets the title of
 * the interpreter window to s
 * also updates the current_interpreter to intprtr
 */
static void set_interpreter_text(char *intprtr, const char *s)
{
	GtkTextBuffer *temp = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(widgets[INTERPRETERTEXT]));
	gtk_text_buffer_set_text(temp, intprtr, -1);
	current_interpreter = intprtr;
	gtk_window_set_title(GTK_WINDOW(widgets[INTERPRETERWINDOW]), s);
}
