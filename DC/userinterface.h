#ifndef USERINTERFACE
#define USERINTERFACE

#include "DCProgram.h"
#define GLADEFILE "DCProgram.glade"

enum {USER_INPUT_LY, USER_INPUT_TY};
enum {
	SHEETNAME, KEYCELL, DOC, DR, AGECORR, INFL, TRM_PERCDEF, DR113,
	SS, STANDARD, ASSETS, PARAGRAPH, PUCTUC, CASHFLOWS, EVALUATEDTH,
	RUNCHOICE, TESTCASEBOX, TESTCASE, OPENDCFILE, SAVEASDCFILE,
	OPENEXCELFILE, WINDOW, INTERPRETERWINDOW, MSGERR, FILENAME, STARTSTOP,
	WIDGET_AMOUNT
}; 

extern const char *const widgetname[WIDGET_AMOUNT]; 
extern GtkWidget *widgets[WIDGET_AMOUNT];

struct user_input *get_user_input(unsigned ui);
void set_user_input(struct user_input *);
void update_user_interface(struct user_input *);
void print_user_input(struct user_input *);
void validateUI(Validator *, struct user_input *);
void validateData(Validator *, struct user_input *);
/* signal functions */
void on_startstopbutton_clicked(GtkButton *, GtkWidget *);
void on_SIinterpreterbutton_clicked(GtkButton *b, gpointer *p);
gboolean on_interpreterwindow_delete_event(GtkWidget *, GdkEvent *, gpointer);
void on_close_button_press_event(void);
void on_openDC_activate(GtkMenuItem *m);
void on_saveDC_activate(GtkMenuItem *m);
void on_saveasDC_activate(GtkMenuItem *m);
void on_LYfilechooserbutton_file_set(GtkFileChooserButton *, gpointer);

#endif
