#include "userinterface.h"
#include "assumptions.h"
#include "printresults.h"

#define NOT_RUNNING 01
#define RUNNING 02
#define INTERRUPTED 04

static _Atomic unsigned run_state = NOT_RUNNING;
static Database *db;

struct gui_data {
	const char *s;
	gpointer pl;
};

extern void runmember(CurrentMember cm[static 1]);
static gpointer run(gpointer pl);
static gpointer runtc(gpointer pl);
static gpointer stoprun(gpointer data);
gboolean update_gui(gpointer data);

static void free_run_garbage(void)
{
	dates_arena_free();
}

const Database *get_database(void)
{
	return db;
}

/*
 * TODO: this function needs to make checks before proceeding with the
 * import. currently it just aborts if an error occurs
 */
static void import_data(gpointer pl)
{
	(void)pl;
	UserInput *const *ui = get_user_input();
	assert(ui);
	const char *file_name = ui[INPUT_FILENAME]->input;
	const char *sheet_name = ui[INPUT_SHEETNAME]->input;
	const char *cell = ui[INPUT_KEYCELL]->input;
	if (!file_name) {
		gtk_label_set_text(GTK_LABEL(pl),
				"No file selected\n"
				"Import failed.");
	} else if (!sheet_name) {
		gtk_label_set_text(GTK_LABEL(pl),
				"No sheet selected\n"
				"Import failed.");
	} else if (!cell) {
		gtk_label_set_text(GTK_LABEL(pl),
				"No cell selected\n"
				"Import failed.");
	} else {
		db = open_database(file_name, sheet_name, cell);
		if (!db) {
			char *buf = 0;
			buf_printf(buf, "Import failed. Check if sheet '%s'"
					" exists.\n" "If it does, it may also "
					"be due to no database in cell %s.",
					sheet_name, cell);
			gtk_label_set_text(GTK_LABEL(pl), buf);
			buf_free(buf);
		} else {
			gtk_label_set_text(GTK_LABEL(pl), "Import complete.");
		}
	}
}

void reset_database(void)
{
	if (db) {
		close_database(db);
		db = 0;
	}
}

void on_import_data_clicked(GtkButton *b, GtkWidget *label)
{
	assert(label);
	printf("[%s] pressed\n", gtk_button_get_label(b));
	set_user_inputs();
	reset_database();
	import_data(label);
}

void on_startstopbutton_clicked(GtkButton *b, GtkWidget *pl)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	gchar *choice = 0;
	GtkDialog *dialog = 0;

	if (run_state & NOT_RUNNING) {

		run_state = RUNNING;
		gtk_image_set_from_icon_name(GTK_IMAGE(widgets[ID_STARTSTOP]),
				"media-playback-stop", GTK_ICON_SIZE_BUTTON);
		set_user_inputs();

		validate_UI();

		if (validate_passed()) {
			choice = gtk_combo_box_text_get_active_text(
					GTK_COMBO_BOX_TEXT(
						widgets[ID_RUNCHOICE]));
			if (strcmp("Run one run", choice) == 0) {
				g_thread_new("run", run, pl);
			} else if (strcmp("Run test case", choice) == 0) {
				g_thread_new("runtc", runtc, pl);
			} else
				die("should never reach here");

			g_free(choice);
		} else {
			dialog = GTK_DIALOG(widgets[ID_MSGERR]);

			gtk_message_dialog_format_secondary_text(
					GTK_MESSAGE_DIALOG(dialog), 
					"%s", validate_error());

			gtk_widget_show(GTK_WIDGET(dialog));
			gtk_dialog_run(dialog); 
			gtk_widget_hide(GTK_WIDGET(dialog));
			run_state = NOT_RUNNING;
			gtk_image_set_from_icon_name(
					GTK_IMAGE(widgets[ID_STARTSTOP]),
					"media-playback-start",
					GTK_ICON_SIZE_BUTTON);
			validate_reset();
		}
	} else {
		run_state = INTERRUPTED;
	}
}

static gpointer run(gpointer pl)
{
	unsigned tc = 0;
	CurrentMember *cm = create_members(db);
	char text[BUFSIZ];
	struct gui_data gd = {"Preparing data...", pl};
#if 0
	g_idle_add(update_gui, &gd);
#endif

	tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[ID_TESTCASE])));
	tc -= 1; // Index is one less than given test case
	assert(tc < db->num_records);

	setassumptions();
	for (size_t i = 0; i < db->num_records; i++) {
		if (run_state & INTERRUPTED) {
			return stoprun(&gd);
		}
		runmember(cm + i);
		snprintf(text, sizeof(text), "Progress: member %lu out of %lu "
				"members complete", i + 1, db->num_records);
#if 0
		gd.s = text;
		g_idle_add(update_gui, &gd);
#endif
		printf("member %ld run\n", i);
	}

	print_results(db, cm);
	print_test_case(cm + tc);

	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name(GTK_IMAGE(widgets[ID_STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	buf_free(cm);
	free_run_garbage();
	return 0;
}

static gpointer runtc(gpointer pl)
{
	(void)pl;
	unsigned tc = 0;
	CurrentMember *members = create_members(db);
	CurrentMember *cm = 0;
	char text[BUFSIZ];
	struct gui_data gd = {"Preparing data...", pl};
#if 0
	g_idle_add(update_gui, &gd);
#endif

	tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[ID_TESTCASE])));
	tc -= 1; // Index is one less than given test case
	assert(tc < db->num_records);
	cm = members + tc;

	printf("testcase: %s chosen\n", cm->key);
	if (run_state & INTERRUPTED) {
		return stoprun(&gd);
	}
	setassumptions();
	assert(cm);
	runmember(cm);
	snprintf(text, sizeof(text), "%s [%u] has been run", cm->key, tc + 1);
#if 0
	gd.s = text;
	g_idle_add(update_gui, &gd);
#endif

	print_test_case(cm);

	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name(GTK_IMAGE(widgets[ID_STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	buf_free(members);
	free_run_garbage();
	return 0;
}

static gpointer stoprun(gpointer data)
{
#if 0
	struct gui_data *gd = data;
	gd->s = "Progress: stopped";
	g_idle_add(update_gui, gd);
#endif
	(void)data;
	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name(GTK_IMAGE(widgets[ID_STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	return 0;
}

gboolean update_gui(gpointer data) {
	struct gui_data *gd = data;
	gtk_label_set_text(GTK_LABEL(gd->pl), gd->s);
	return FALSE;
}
