#include "userinterface.h"
#include "assumptions.h"

#define NOT_RUNNING 01
#define RUNNING 02
#define INTERRUPTED 04

static _Atomic unsigned run_state = NOT_RUNNING;

struct gui_data {
	char *s;
	gpointer pl;
};

extern void runmember(CurrentMember cm[static 1]);
static gpointer run(gpointer pl);
static gpointer runtc(gpointer pl);
static gpointer stoprun(gpointer data);
gboolean update_gui(gpointer data);

void on_startstopbutton_clicked(GtkButton *b, GtkWidget *pl)
{
	printf("[%s] pressed\n", gtk_button_get_label(b));
	Validator validatorLY = (Validator) {0};
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	char *MsgErr = 0;
	gchar *choice = 0;
	GtkDialog *dialog = 0;

	if (run_state & NOT_RUNNING) {

		run_state = RUNNING;
		gtk_image_set_from_icon_name(GTK_IMAGE(widgets[STARTSTOP]),
				"media-playback-stop", GTK_ICON_SIZE_BUTTON);
		set_user_input(ui);
		validatorLY.status = OK;
		validateUI(&validatorLY, ui); 
		validateData(&validatorLY, ui);

		if (validatorLY.status != ERROR) {
			choice = gtk_combo_box_text_get_active_text(
					GTK_COMBO_BOX_TEXT(
						widgets[RUNCHOICE]));
			if (strcmp("Run one run", choice) == 0) {
				// This needs updating when I start with reconciliation runs!!
				currrun = runNewRF; 
				g_thread_new("run", run, pl);
			} else if (strcmp("Run test case", choice) == 0) {
				// This needs updating when I start with reconciliation runs!!
				currrun = runNewRF; 
				g_thread_new("runtc", runtc, pl);
			} else if (strcmp("Run reconciliation", choice) == 0) {
				printf("something else\n");
			} else
				die("should never reach here");

			g_free(choice);
		} else {
			MsgErr = setMsgbuf(&validatorLY);
			dialog = GTK_DIALOG(widgets[MSGERR]);

			gtk_message_dialog_format_secondary_text(
					GTK_MESSAGE_DIALOG(dialog), 
					"%s", MsgErr);

			gtk_widget_show(GTK_WIDGET(dialog));
			gtk_dialog_run(dialog); 
			gtk_widget_hide(GTK_WIDGET(dialog));
			run_state = NOT_RUNNING;
			gtk_image_set_from_icon_name(
					GTK_IMAGE(widgets[STARTSTOP]),
					"media-playback-start",
					GTK_ICON_SIZE_BUTTON);
		}
	} else {
		run_state = INTERRUPTED;
	}
}

static gpointer run(gpointer pl)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	unsigned tc = 0;
	DataSet *ds = 0;
	CurrentMember *cm = 0;
	char text[BUFSIZ];
	struct gui_data gd = {"Preparing data...", pl};
	g_idle_add(update_gui, &gd);

	tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[TESTCASE])));
	tc -= 1; // Index is one less than given test case
	ds = createDS(0, ui);
	cm = ds->cm;

	/* this needs updating when we have a UITY!!! */
	setassumptions(ui, ui);
	for (unsigned i = 0; i < ds->membercnt; i++) {
		if (run_state & INTERRUPTED) {
			freeDS(ds);
			return stoprun(&gd);
		}
		runmember(cm + i);
		snprintf(text, sizeof(text), "Progress: member %u out of %u "
				"members complete", i + 1, ds->membercnt);
		gd.s = text;
		g_idle_add(update_gui, &gd);
	}

	printresults(ds);
	printtc(ds, tc);

	freeDS(ds);
	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name(GTK_IMAGE(widgets[STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	return 0;
}

static gpointer runtc(gpointer pl)
{
	struct user_input *ui = get_user_input(USER_INPUT_LY);
	unsigned tc = 0;
	DataSet *ds = 0;
	CurrentMember *cm = 0;
	char text[BUFSIZ];
	struct gui_data gd = {"Preparing data...", pl};
	g_idle_add(update_gui, &gd);

	tc = atoi(gtk_entry_get_text(GTK_ENTRY(widgets[TESTCASE])));
	tc -= 1; // Index is one less than given test case
	ds = createDS(0, ui);
	cm = ds->cm + tc;

	printf("testcase: %s chosen\n", cm->key);
	if (run_state & INTERRUPTED) {
		freeDS(ds);
		return stoprun(&gd);
	}
	/* this needs updating when we have a UITY!!! */
	setassumptions(ui, ui);
	runmember(cm);
	snprintf(text, sizeof(text), "Test case %u has been run", tc + 1);
	gd.s = text;
	g_idle_add(update_gui, &gd);

	printtc(ds, tc);

	freeDS(ds);
	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name(GTK_IMAGE(widgets[STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	return 0;
}

static gpointer stoprun(gpointer data)
{
	struct gui_data *gd = data;
	gd->s = "Progress: stopped";
	g_idle_add(update_gui, gd);
	run_state = NOT_RUNNING;
	gtk_image_set_from_icon_name(GTK_IMAGE(widgets[STARTSTOP]),
			"media-playback-start", GTK_ICON_SIZE_BUTTON);
	return 0;
}

gboolean update_gui(gpointer data) {
	struct gui_data *gd = data;
	gtk_label_set_text(GTK_LABEL(gd->pl), gd->s);
	return FALSE;
}
