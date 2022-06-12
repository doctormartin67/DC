#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include "type.h"
#include "DCProgram.h"
#define GLADEFILE "glade/DCProgram.glade"

typedef enum {
	ID_SHEETNAME,
	ID_KEYCELL,
	ID_DOC,
	ID_DR,
	ID_AGECORR,
	ID_INFL,
	ID_TRM_PERCDEF,
	ID_DR113,
	ID_INTERPRETERTEXT,
	ID_STANDARD,
	ID_ASSETS,
	ID_PUCTUC,
	ID_MAXPUCTUC,
	ID_MAXERCONTR,
	ID_EVALUATEDTH,
	ID_RUNCHOICE,
	ID_TESTCASEBOX,
	ID_TESTCASE,
	ID_OPENDCFILE,
	ID_SAVEASDCFILE,
	ID_FILENAME,
	ID_WINDOW,
	ID_INTERPRETERWINDOW,
	ID_MSGERR,
	ID_FILENAME_LABEL,
	ID_STARTSTOP,
} WidgetId;

typedef enum {
	WIDGET_ENTRY,
	WIDGET_COMBO_BOX,
	WIDGET_INTERPRETER,
	WIDGET_FILE_CHOOSER,
} WidgetKind;

typedef enum {
	// Interpreter
	INPUT_SS,
	INPUT_TURNOVER,
	INPUT_RETX,
	INPUT_NRA,
	INPUT_ADMINCOST,
	INPUT_COSTRES,
	INPUT_COSTKO,
	INPUT_WD,
	INPUT_PREPOST,
	INPUT_TERM,
	INPUT_LTINS,
	INPUT_LTTERM,
	INPUT_CONTRA,
	INPUT_CONTRC,

	// Entry
	INPUT_SHEETNAME,
	INPUT_DOC,
	INPUT_DR,
	INPUT_AGECORR,
	INPUT_INFL,
	INPUT_TRM_PERCDEF,
	INPUT_DR113,

	//Combo
	INPUT_STANDARD,
	INPUT_ASSETS,
	INPUT_PUCTUC,
	INPUT_MAXPUCTUC,
	INPUT_MAXERCONTR,
	INPUT_EVALUATEDTH,

	// Other
	INPUT_KEYCELL,
	INPUT_FILENAME,
} InputKind;

typedef struct UserInput {
	WidgetKind widget_kind;
	InputKind input_kind;
	WidgetId id;
	const char *name;
	const char *input;
	TypeKind return_type;
} UserInput;

extern GtkWidget **widgets;

void userinterface(void);
UserInput **get_user_input(void);
void set_user_inputs(void);
void update_user_interface(void);
void validate_UI(void); 
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
