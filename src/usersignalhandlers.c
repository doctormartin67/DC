#include <assert.h>
#include "userinterface.h"

#define LHEADER "<<<<<"
#define RHEADER ">>>>>"

extern void reset_database(void);

/*
 * This equals the current index of the interpreter that will be set by the
 * open interpreter window once it closes
 */
static unsigned current_interpreter;

static void set_interpreter_text(unsigned intprtr);

void on_close_button_press_event(void)
{
	reset_database();
	gtk_main_quit();
}

static void read_user_input(const char *file_name)
{
	Map *user_input = get_user_input();
	FILE *fp = fopen(file_name, "r");
	if (!fp) die("Unable to read file [%s]", file_name);
	size_t len = BUFSIZ;
	char line[len];
	char value[len];
	char *key = 0;
	char *keyend = 0;
	char *pl = line;
	char *pv = value;
	long fppos = 0;

	while (0 != fgets(line, len, fp)) {
		if (0 == (key = strinside(line, LHEADER, RHEADER))) continue;
		keyend = strstr(key, RHEADER);
		assert(0 != keyend);
		*keyend = '\0';
		key = strdup(key);

		while (0 != fgets(line, len, fp)) {
			if (strstr(line, LHEADER)) break;
			while (*pl && pv < value + len) *pv++ = *pl++;
			pl = line;
			if (-1 == (fppos = ftell(fp)))
				die("Unable to find position");
		}

		if (pv > value)
			*(pv - 1) = '\0';
		else
			*pv = '\0';

		map_put_str(user_input, key, strdup(value));

		pv = value;
		fseek(fp, fppos, SEEK_SET);
	}

	if (0 != fclose(fp)) die("Unable to close file");
}

static void write_user_input(const char *restrict file_name)
{
	Map *user_input = get_user_input();
	FILE *fp = fopen(file_name, "w");
	if (!fp) die("Unable to open/create file [%s]", file_name);
	char *buf = 0;
	char *key = 0;

	for (size_t i = 0; i < UI_AMOUNT; i++) {
		key = strdup(get_ui_key(i, UI_INT));
		upper(key);
		buf_printf(buf, "%s%s%s\n", LHEADER, key, RHEADER);
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
		buf_printf(buf, "%s\n", map_get_str(user_input, key));
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
	}
	for (size_t i = 0; i < UI_FIXED_AMOUNT; i++) {
		key = strdup(get_ui_key(i, UI_FIXED));
		upper(key);
		buf_printf(buf, "%s%s%s\n", LHEADER, key, RHEADER);
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
		buf_printf(buf, "%s\n", map_get_str(user_input, key));
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
	}
	for (size_t i = 0; i < COMBO_AMOUNT; i++) {
		key = strdup(get_ui_key(i, UI_COMBO));
		upper(key);
		buf_printf(buf, "%s%s%s\n", LHEADER, key, RHEADER);
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
		buf_printf(buf, "%s\n", map_get_str(user_input, key));
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
	}
	for (size_t i = 0; i < SPECIAL_AMOUNT; i++) {
		key = strdup(get_ui_key(i, UI_SPECIAL));
		upper(key);
		buf_printf(buf, "%s%s%s\n", LHEADER, key, RHEADER);
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
		buf_printf(buf, "%s\n", map_get_str(user_input, key));
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
	}

	if (0 != fclose(fp)) die("Unable to close file");
}

/*
 * Open .dc file that will fill in the current user interface
 */
void on_openDC_activate(GtkMenuItem *m)
{
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

		read_user_input(filename);
		update_user_interface();

		g_free(filename);
		filename = 0;
	} else {
		printf("Cancelled [%s]\n", gtk_menu_item_get_label(m));
	}

	gtk_widget_hide(GTK_WIDGET(dialog));
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
	char tmp[BUFSIZ];
	char *filename = 0;
	char *p = 0;
	GtkDialog *dialog = 0;
	GtkFileChooser *chooser = 0;
	gint res = 0;

	set_user_input();
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

		write_user_input(tmp);

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
	Map *user_input = get_user_input();
	if (0 != p) printf("unused pointer [%p]\n", p);
	printf("dialog [%s] closed\n", gtk_file_chooser_button_get_title(b));

	GtkDialog *dialog = 0;
	const char *key = 0;
	char *filename = 0;
	char *tmp = 0;

	dialog = GTK_DIALOG(widgets[OPENEXCELFILE]);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	filename = gtk_file_chooser_get_filename(chooser);

	key = get_ui_key(SPECIAL_FILENAME, UI_SPECIAL);
	map_put_str(user_input, key, strdup(filename));
	buf_printf(tmp, "File set to run:\n%s", filename);

	printf("Excel to run: [%s]\n", filename);
	gtk_label_set_text(GTK_LABEL(widgets[FILENAME]), tmp); 
	g_free(filename);
	buf_free(tmp);
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
	Map *user_input = get_user_input();
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

	map_put_str(user_input, get_ui_key(current_interpreter, UI_INT),
			strdup(s));
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
	Map *user_input = get_user_input();
	char *s = strdup(get_ui_key(intprtr, UI_INT));
	upper(s);
	const char *t = map_get_str(user_input, s);
	printf("s = %s\n", s);
	printf("t = %s\n", t);
	GtkTextBuffer *temp = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(widgets[INTERPRETERTEXT]));

	if (t) gtk_text_buffer_set_text(temp, t, -1);
	else gtk_text_buffer_set_text(temp, "", -1);

	current_interpreter = intprtr;
	assert(current_interpreter < UI_AMOUNT);
	gtk_window_set_title(GTK_WINDOW(widgets[INTERPRETERWINDOW]), s);
}
