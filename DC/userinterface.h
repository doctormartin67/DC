#ifndef USERINTERFACE
#define USERINTERFACE

#include "DCProgram.h"
#define GLADEFILE "DCProgram.glade"

enum {
	SHEETNAME, KEYCELL, DOC, DR, AGECORR, INFL, TRM_PERCDEF, DR113,
	FIXEDSIENTRY, SS, STANDARD, ASSETS, PARAGRAPH, PUCTUC, CASHFLOWS,
	EVALUATEDTH, FIXEDSIRADIOBUTTON, RUNCHOICE, TESTCASEBOX, TESTCASE,
	OPENDCFILE, SAVEASDCFILE, OPENEXCELFILE, WINDOW, ASSWINDOW, MSGERR,
	FILENAME, WIDGET_AMOUNT
}; 

extern const char *const widgetname[WIDGET_AMOUNT]; 

/* signal functions */
void on_SIradiobutton_toggled(GtkRadioButton *, GtkWidget *);
void on_startstopbutton_clicked(GtkButton *, GtkWidget *);
void on_SIinterpreterbutton_clicked(GtkButton *b, gpointer *p);
void on_runchoice_changed(GtkComboBox *cb, gpointer *p);
gboolean on_asswindow_delete_event(GtkWidget *, GdkEvent *, gpointer);
void on_close_button_press_event(void);
void on_openDC_activate(GtkMenuItem *m);
void on_saveDC_activate(GtkMenuItem *m);
void on_saveasDC_activate(GtkMenuItem *m);
void on_LYfilechooserbutton_file_set(GtkFileChooserButton *, gpointer);

/* helper functions */
static GtkWidget *buildWidget(const char *);
static void *run(void *);
static void *runtc(void *pl);
static void setUIvals(UserInput *UI);
static void updateUI(UserInput *UI);
static void printUI(UserInput *UI);
static void validateUI(Validator *, UserInput *);
static void validateData(Validator *, UserInput *);

#endif
