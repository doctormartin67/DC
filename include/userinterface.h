#ifndef USERINTERFACE
#define USERINTERFACE

#include "DCProgram.h"
#define GLADEFILE "glade/DCProgram.glade"

enum {USER_INPUT_LY, USER_INPUT_TY};
enum {
	SHEETNAME, KEYCELL, W_DOC_LY, DR, AGECORR, INFL, TRM_PERCDEF, DR113,
	INTERPRETERTEXT, STANDARD, W_ASSETS_LY, PUCTUC, MAXPUCTUC, MAXERCONTR,
	EVALUATEDTH, RUNCHOICE, TESTCASEBOX, TESTCASE, OPENDCFILE,
	SAVEASDCFILE, OPENEXCELFILE, WINDOW, INTERPRETERWINDOW, MSGERR,
	FILENAME, STARTSTOP, WIDGET_AMOUNT
}; 

/*
 * interpreter indexes, meaning the variables that use an interpreter to be
 * determined
 */
enum {
	UI_SS, UI_TURNOVER, UI_RETX, UI_NRA,
	UI_ADMINCOST, UI_COSTRES, UI_COSTKO, UI_WD, UI_PREPOST, UI_TERM,
	UI_LTINS, UI_LTTERM,
	UI_CONTRA, UI_CONTRC,
	UI_AMOUNT
};

/*
 * indexes used for the variables in the user interface that are fixed
 */
enum {
	UI_SHEETNAME, UI_DOC, UI_DR, UI_AGECORR, UI_INFL, UI_TRM_PERCDEF,
	UI_DR113, UI_FIXED_AMOUNT
};

/*
 * indexes used for user input that use combo boxes to determine which method
 * is used
 */
enum {
	COMBO_STANDARD, COMBO_ASSETS, COMBO_PUCTUC, COMBO_MAXPUCTUC,
	COMBO_MAXERCONTR, COMBO_EVALDTH, COMBO_AMOUNT
};

/*
 * indexes used for user input that have some special maniulation to be done
 */
enum {
	SPECIAL_KEYCELL, SPECIAL_FILENAME, SPECIAL_AMOUNT
};

enum {UI_INT, UI_FIXED, UI_COMBO, UI_SPECIAL};

struct user_input {
	const char *const key;
	unsigned widget;
};

extern const char *const widgetname[WIDGET_AMOUNT]; 
extern GtkWidget *widgets[WIDGET_AMOUNT];

const char *get_ui_key(unsigned var, unsigned type);
unsigned get_ui_widget(unsigned var, unsigned type);
Hashtable *get_user_input(unsigned ui);
void set_user_input(Hashtable *);
void update_user_interface(Hashtable *);
void validateUI(Validator *, Hashtable *);
void validateData(Validator *, Hashtable *);
/* signal functions */
void on_startstopbutton_clicked(GtkButton *, GtkWidget *);
gboolean on_interpreterwindow_delete_event(GtkWidget *, GdkEvent *, gpointer);
void on_close_button_press_event(void);
void on_openDC_activate(GtkMenuItem *m);
void on_saveDC_activate(GtkMenuItem *m);
void on_saveasDC_activate(GtkMenuItem *m);
void on_LYfilechooserbutton_file_set(GtkFileChooserButton *, gpointer);

void on_SS_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_turnover_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_retx_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_NRA_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_admincost_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_costRES_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_costKO_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_WD_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_prepost_interpreterbutton_clicked(GtkButton *b, gpointer *w);
void on_term_interpreterbutton_clicked(GtkButton *b, gpointer *w);

#endif
