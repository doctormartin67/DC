#include <assert.h>
#include "userinterface.h"
#include "helperfunctions.h"
#include "errorexit.h"

const Database *get_database(void);

const char *const widget_names[] = {
	[ID_SHEETNAME] = "sheetname",
	[ID_KEYCELL] = "keycell",
	[ID_DOC] = "DOC",
	[ID_DR] = "DR",
	[ID_AGECORR] = "agecorr",
	[ID_INFL] = "infl",
	[ID_TRM_PERCDEF] = "TRM_PercDef",
	[ID_DR113] = "DR113",
	[ID_INTERPRETERTEXT] = "interpretertext",
	[ID_STANDARD] = "standard",
	[ID_ASSETS] = "assets",
	[ID_PUCTUC] = "PUCTUC",
	[ID_MAXPUCTUC] = "maxPUCTUC",
	[ID_MAXERCONTR] = "maxERContr",
	[ID_EVALUATEDTH] = "evaluateDTH",
	[ID_RUNCHOICE] = "runchoice",
	[ID_TESTCASEBOX] = "testcasebox",
	[ID_TESTCASE] = "testcase",
	[ID_OPENDCFILE] = "openDCFile",
	[ID_SAVEASDCFILE] = "saveasDCFile",
	[ID_FILENAME] = "openExcelFile",
	[ID_WINDOW] = "window",
	[ID_INTERPRETERWINDOW] = "interpreterwindow",
	[ID_MSGERR] = "MsgErr",
	[ID_FILENAME_LABEL] = "filename",
	[ID_STARTSTOP] = "startstop"
};

GtkWidget **widgets;
static UserInput **user_input;
static GtkBuilder *builder;

UserInput **get_user_input(void)
{
	assert(user_input);
	return user_input;
}

static GtkWidget *buildWidget(const char w[static 1])
{
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, w));
	if (0 == widget) die("incorrect builder name");
	return widget;
}

static void init_window(void)
{
	gtk_builder_connect_signals(builder, 0);
	g_signal_connect(widgets[ID_WINDOW], "destroy",
			G_CALLBACK(gtk_main_quit), 0);

	gtk_widget_show(widgets[ID_WINDOW]);
}

static void init_widgets(void)
{
	builder = gtk_builder_new_from_file(GLADEFILE);

	size_t num_widgets = sizeof(widget_names)/sizeof(widget_names[0]);
	for (size_t i = 0; i < num_widgets; i++) {
		buf_push(widgets, buildWidget(widget_names[i]));
	}
}

Arena ui_arena;
static UserInput *new_user_input(WidgetKind widget_kind, InputKind input_kind,
		WidgetId id, const char *name, const char *input)
{
	UserInput *ui = arena_alloc(&ui_arena, sizeof(*ui));	
	ui->widget_kind = widget_kind;
	ui->input_kind = input_kind;
	ui->id = id;
	ui->name = name;
	ui->input = input;
	return ui;
}

#define USER_INPUT_I(input, name) \
	buf_push(user_input, new_user_input(WIDGET_INTERPRETER, INPUT_##input, \
				ID_INTERPRETERTEXT, name, 0));
#define USER_INPUT_E(i, name) \
	buf_push(user_input, new_user_input(WIDGET_ENTRY, INPUT_##i, \
				ID_##i, name, 0));
#define USER_INPUT_C(i, name) \
	buf_push(user_input, new_user_input(WIDGET_COMBO_BOX, INPUT_##i, \
				ID_##i, name, 0));
#define USER_INPUT_F(i, name) \
	buf_push(user_input, new_user_input(WIDGET_FILE_CHOOSER, INPUT_##i, \
				ID_##i, name, 0));

/*
 * these buf_pushes need to be in the same order as the enum InputKind!
 */
static void init_user_input(void)
{
	USER_INPUT_I(SS, "Salary Scale");
	USER_INPUT_I(TURNOVER, "Turnover");
	USER_INPUT_I(RETX, "Retirement Probability");
	USER_INPUT_I(NRA, "Normal Retirement Age");
	USER_INPUT_I(ADMINCOST, "Administration Cost");
	USER_INPUT_I(COSTRES, "Cost on Reserves");
	USER_INPUT_I(COSTKO, "Cost on Lump Sum Life");
	USER_INPUT_I(WD, "Profit Sharing for Mortality");
	USER_INPUT_I(PREPOST, "Immediate or Due payments");
	USER_INPUT_I(TERM, "Payment Frequency");
	USER_INPUT_I(LTINS, "Life Tables Insurer");
	USER_INPUT_I(LTTERM, "Life Tables After Termination");
	USER_INPUT_I(CONTRA, "Employer Contribution");
	USER_INPUT_I(CONTRC, "Employee Contribution");

	USER_INPUT_E(SHEETNAME, "Sheet name");
	USER_INPUT_E(DOC, "DOC");
	USER_INPUT_E(DR, "DR");
	USER_INPUT_E(AGECORR, "Age Correction");
	USER_INPUT_E(INFL, "Inflation");
	USER_INPUT_E(TRM_PERCDEF, "Termination percentage");
	USER_INPUT_E(DR113, "DR $113");

	USER_INPUT_C(STANDARD, "Standard IAS/FAS");
	USER_INPUT_C(ASSETS, "Assets");
	USER_INPUT_C(PUCTUC, "PUC/TUC");
	USER_INPUT_C(MAXPUCTUC, "max(PUC, TUC)");
	USER_INPUT_C(MAXERCONTR, "max(NC, ER Contr)");
	USER_INPUT_C(EVALUATEDTH, "Evaluate Death");

	USER_INPUT_E(KEYCELL, "Key Cell");
	USER_INPUT_F(FILENAME, "File Name");
}

#undef USER_INPUT_I
#undef USER_INPUT_E
#undef USER_INPUT_C
#undef USER_INPUT_F

static void init_interface(void)
{
	init_widgets();
	init_user_input();
	init_window();
}

void userinterface(void)
{
	gtk_init(0, 0);
	init_interface();
	gtk_main();
}

static const char *get_entry(WidgetId id)
{
	const char *input = gtk_entry_get_text(GTK_ENTRY(widgets[id]));
	assert(input);
	return arena_str_dup(&ui_arena, input);
}

static const char *get_combo(WidgetId id)
{
	char *buf = 0;
	const char *input = 0;
	buf_printf(buf, "%d", gtk_combo_box_get_active(GTK_COMBO_BOX(
					widgets[id])));
	input = arena_str_dup(&ui_arena, buf);
	buf_free(buf);
	assert(input);
	return input;
}

static void set_user_input(UserInput *ui)
{
	switch (ui->widget_kind) {
		case WIDGET_ENTRY:
			ui->input = get_entry(ui->id);
			break;
		case WIDGET_COMBO_BOX:
			ui->input = get_combo(ui->id);
			break;
		case WIDGET_FILE_CHOOSER:
		case WIDGET_INTERPRETER:
			// done in usersignalhandlers.c
			break;
		default:
			assert(0);
	}
}

void set_user_inputs(void)
{
	for (size_t i = 0; i < buf_len(user_input); i++) {
		set_user_input(user_input[i]);
	}
}

void set_user_interface_input(const UserInput *ui)
{
	WidgetId id = ui->id;
	const char *input = ui->input;
	switch (ui->widget_kind) {
		case WIDGET_ENTRY:
			gtk_entry_set_text(GTK_ENTRY(widgets[id]), input);
			break;
		case WIDGET_COMBO_BOX:
			gtk_combo_box_set_active(GTK_COMBO_BOX(widgets[id]),
					atoi(input));
			break;
		case WIDGET_FILE_CHOOSER:
			id = ID_FILENAME_LABEL;
			gtk_label_set_text(GTK_LABEL(widgets[id]), input); 
			break;
		case WIDGET_INTERPRETER:
			// done in usersignalhandlers.c
			break;
		default:
			assert(0);
	}
	printf("%s: %s\n", ui->name, ui->input);
}

void update_user_interface(void)
{
	for (size_t i = 0; i < buf_len(user_input); i++) {
		set_user_interface_input(user_input[i]);
	}
}

#define ERROR(s, name) \
		validate_emit_error("input '%s' invalid for '%s'", s, name);

static void validate_keycell(void)
{

	unsigned num_cols = 0;
	size_t len = 0;
	const char *kc = user_input[INPUT_KEYCELL]->input;
	const char *pt = 0;
	assert(kc);
	len = strlen(kc);

	if (kc[0] < 'A' || kc[0] > 'Z') {
		ERROR(kc, "Data cell");
	} else if (len > 1) {
		pt = kc + 1;
		num_cols++;

		while (!isdigit(*pt)) {
			pt++;
			num_cols++;
		}

		if (num_cols > 3) {
			ERROR(kc, "Data cell");
		}

		if ('\0' == *pt) {
			ERROR(kc, "Data cell");
		}

		while (isdigit(*pt)) pt++;

		if ('\0' != *pt) {
			ERROR(kc, "Data cell");
		}
	} else if (1 == len) {
		ERROR(kc, "Data cell");
	}
}

static void validate_date(const char *name, InputKind kind)
{
	const char *value = 0;
	char *day = 0, *month = 0, *year = 0;
	char *buf = 0;
	struct date *tmp_date = 0;

	value = user_input[kind]->input;
	assert(value);
	buf_printf(buf, "%s", value);

	day = strtok(buf, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	if (0 == day || 0 == month || 0 == year) {
		ERROR(value, name);
	} else {
		if (!isint(day) || !isint(month) || !isint(year)) {
			ERROR(value, name);
		}

		tmp_date = newDate(0, atoi(year), atoi(month), atoi(day));
		if (0 == tmp_date) {
			ERROR(value, name);
		}
	}
	buf_free(buf);
}

#define VALIDATE(kind, type, name) \
	value = user_input[kind]->input; \
	if (!is##type(value)) { \
		ERROR(value, name); \
	}

static unsigned validate_input_set(void)
{
	for (size_t i = 0; i < buf_len(user_input); i++) {
		if (!user_input[i]->input) {
			validate_emit_error("%s undefined",
					user_input[i]->name);
		}
	}
	if (validate_passed()) {
		return 1;
	} else {
		return 0;
	}
}

static void validate_test_case(void)
{
	const char *test_case = 0;
	int tc = 0;
	test_case = gtk_entry_get_text(GTK_ENTRY(widgets[ID_TESTCASE]));

	tc = atoi(test_case);
	if (!isint(test_case) || tc < 1) {
		ERROR(test_case, "test case");
	}

	const Database *db = get_database();
	if (db && isint(test_case) && !(tc < 1)) {
		if ((size_t)tc > db->num_records) {
			validate_emit_error("test case '%d' is larger than the "
					"amount of records '%d' found in the "
					"database.", tc, db->num_records);
		}
	}
}

void validate_UI(void)
{
	if (!validate_input_set()) {
		return;
	}

	if (!get_database()) {
		validate_emit_error("database has not been imported");
	}

	if (!user_input[INPUT_FILENAME]->input) {
		validate_emit_error("No file selected to run");
	}

	validate_keycell();
	validate_date("DOC", INPUT_DOC);
	validate_test_case();

	const char *value = 0;
	VALIDATE(INPUT_DR, float, "DR");
	VALIDATE(INPUT_INFL, float, "Inflation");
	VALIDATE(INPUT_AGECORR, int, "Age Correction");
	VALIDATE(INPUT_TRM_PERCDEF, float, "Termination");
	VALIDATE(INPUT_DR113, float, "DR $113");
}

#undef ERROR
#undef VALIDATE
