#include <assert.h>
#include "userinterface.h"

#define LHEADER "<<<<<"
#define RHEADER ">>>>>"

extern Arena ui_arena;
extern void reset_database(void);

/*
 * This equals the current index of the interpreter that will be set by the
 * open interpreter window once it closes
 */
static unsigned current_interpreter;

static void set_interpreter_text(size_t i);

static void free_ui_garbage(void)
{
	reset_database();
	arena_free(&ui_arena);
	UserInput **ui = get_user_input();
	buf_free(ui);;
	buf_free(widgets);
}

void on_close_button_press_event(void)
{
	free_ui_garbage();
	gtk_main_quit();
}

static void set_user_input(const char *name, const char *input)
{
	UserInput *const *ui = get_user_input();
	for (size_t i = 0; i < buf_len(ui); i++) {
		assert(ui[i]->name);
		if (!strcmp(ui[i]->name, name)) {
			ui[i]->input = arena_str_dup(&ui_arena, input);
		}
	}
}

static char *strdup_inside(const char *str, const char *left,
		const char *right)
{
	char *str_new = 0;
	char *end = 0;
	assert(str);
	assert(left);
	assert(right);

	if (0 == (str = strinside(str, left, right))) {
		return 0;
	}
	
	str_new = strdup(str);

	end = strstr(str_new, right);
	assert(0 != end);
	*end = '\0';
	return str_new;
}

static void read_user_input(const char *file_name)
{
	FILE *fp = fopen(file_name, "r");
	if (!fp) die("Unable to read file [%s]", file_name);
	char line[BUFSIZ];
	char *key = 0;
	long fppos = 0;

	while (0 != fgets(line, sizeof(line), fp)) {
		if (0 == (key = strdup_inside(line, LHEADER, RHEADER))) {
			continue;
		}
		char *buf = 0;

		while (0 != fgets(line, sizeof(line), fp)) {
			if (strstr(line, LHEADER)) break;
			buf_printf(buf, "%s", line);
			if (-1 == (fppos = ftell(fp)))
				die("Unable to find position");
		}

		assert('\n' == *(buf + strlen(buf) - 1));
		*(buf + strlen(buf) - 1) = '\0';
		if ('\r' == *(buf + strlen(buf) - 2)) {
			*(buf + strlen(buf) - 2) = '\0'; // windows garbage
		}

		set_user_input(key, buf);

		fseek(fp, fppos, SEEK_SET);
		buf_free(buf);
		free(key);
	}

	if (0 != fclose(fp)) die("Unable to close file");
}

static void write_user_input(const char *restrict file_name)
{
	FILE *fp = fopen(file_name, "w");
	if (!fp) die("Unable to open/create file [%s]", file_name);

	const char *name = 0;
	const char *input = 0;
	char *buf = 0;
	UserInput *const *ui = get_user_input();

	for (size_t i = 0; i < buf_len(ui); i++) {
		name = ui[i]->name;
		input = ui[i]->input;
		buf_printf(buf, "%s%s%s\n", LHEADER, name, RHEADER);
		if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		buf_free(buf);
		buf_printf(buf, "%s\n", input);
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

	dialog = GTK_DIALOG(widgets[ID_OPENDCFILE]);
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

static unsigned has_dc_ext(const char *str)
{
	size_t len = strlen(str);
	if (len < 3) {
		return 0;
	}
	str += len;
	if ('c' == str[-1] && 'd' == str[-2] && '.' == str[-3]) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * save .dc file as given name with the current user input values
 */
void on_saveasDC_activate(GtkMenuItem *m)
{
	gchar *filename = 0;
	GtkDialog *dialog = 0;
	GtkFileChooser *chooser = 0;
	gint res = 0;

	set_user_inputs();
	dialog = GTK_DIALOG(widgets[ID_SAVEASDCFILE]);
	gtk_widget_show(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog); 

	if (res == GTK_RESPONSE_ACCEPT) {
		char *buf = 0;
		chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);

		buf_printf(buf, "%s", filename);
		if (!has_dc_ext(filename)) {
			buf_printf(buf, ".dc");
		}
		printf("selected file [%s] to save\n", buf);

		write_user_input(buf);

		buf_free(buf);
		g_free(filename);
		filename = 0;
	} else {
		printf("Cancelled [%s]\n", gtk_menu_item_get_label(m));
	}

	gtk_widget_hide(GTK_WIDGET(dialog));
}

static const char *get_chooser(WidgetId id)
{
	GtkDialog *dialog = GTK_DIALOG(widgets[id]);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	const char *input = gtk_file_chooser_get_filename(chooser);
	if (!input) {
		printf("Warning: file name set to null\n");
		return 0;
	}
	return arena_str_dup(&ui_arena, input);
}

/*
 * sets the file name of the user input to the chosen excel file
 */
void on_LYfilechooserbutton_file_set(GtkFileChooserButton *b, gpointer p)
{
	if (0 != p) printf("unused pointer [%p]\n", p);
	printf("dialog [%s] closed\n", gtk_file_chooser_button_get_title(b));

	char *tmp = 0;
	const char *name = 0;
	UserInput *const *ui = get_user_input();

	ui[INPUT_FILENAME]->input = get_chooser(ID_FILENAME);
	name = ui[INPUT_FILENAME]->input;
	buf_printf(tmp, "File set to run:\n%s", name);

	printf("Excel to run: [%s]\n", name);
	gtk_label_set_text(GTK_LABEL(widgets[ID_FILENAME_LABEL]), tmp); 
	buf_free(tmp);
}

static void set_input_interpreter(UserInput *ui)
{
	assert(ID_INTERPRETERTEXT == ui->id);
	assert(WIDGET_INTERPRETER == ui->widget_kind);

	GtkTextBuffer *temp = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(widgets[ID_INTERPRETERTEXT]));
	gchar *s = 0;
	GtkTextIter begin, end;
	gtk_text_buffer_get_iter_at_offset(temp, &begin, 0);
	gtk_text_buffer_get_iter_at_offset(temp, &end, -1);
	s = gtk_text_buffer_get_text(temp, &begin, &end, TRUE);

	ui->input = arena_str_dup(&ui_arena, s);
	g_free(s);
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

	UserInput *const *ui = get_user_input();
	set_input_interpreter(ui[current_interpreter]);
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
	set_interpreter_text(INPUT_SS);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the turnover
 * array address
 */
void on_turnover_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_TURNOVER);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the retx
 * array address
 */
void on_retx_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_RETX);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * opens the interpreter and sets current_interpreter to the NRA
 * array address
 */
void on_NRA_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_NRA);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_admincost_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_ADMINCOST);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_costRES_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_COSTRES);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_costKO_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_COSTKO);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_WD_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_WD);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_prepost_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_PREPOST);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_term_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_TERM);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_ltINS_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_LTINS);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_ltTERM_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_LTTERM);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_contrA_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_CONTRA);
	gtk_widget_show_all(GTK_WIDGET(w));
}

void on_contrC_interpreterbutton_clicked(GtkButton *b, gpointer *w)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_interpreter_text(INPUT_CONTRC);
	gtk_widget_show_all(GTK_WIDGET(w));
}

/*
 * sets the interpreter text to the given interpreter and sets the title of
 * the interpreter window to s
 * also updates the current_interpreter to intprtr
 */
static void set_interpreter_text(size_t i)
{
	const UserInput *ui = get_user_input()[i];
	assert(ui);
	assert(WIDGET_INTERPRETER == ui->widget_kind);
	const char *name = ui->name;
	const char *input = ui->input;
	GtkTextBuffer *temp = gtk_text_view_get_buffer(
			GTK_TEXT_VIEW(widgets[ID_INTERPRETERTEXT]));

	if (input) gtk_text_buffer_set_text(temp, input, -1);
	else gtk_text_buffer_set_text(temp, "", -1);

	current_interpreter = i;
	gtk_window_set_title(GTK_WINDOW(widgets[ID_INTERPRETERWINDOW]), name);
}
