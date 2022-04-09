#include <assert.h>
#include <math.h>
#include "type.h"
#include "assumptions.h"
#include "lifetables.h"
#include "interpret.h"
#include "resolve.h"

extern const Database *get_database(void);
enum {VAR_INTERPRETER, VAR_FIXED, VAR_COMBO};
static const char *get_var(unsigned ui, unsigned var_type,
		Hashtable ht[static 1]);
static void set_methodology(Hashtable ht[static 1]);

static void add_builtin_vars(const CurrentMember *cm, int k)
{
	add_builtin_double("infl", ass.infl);
	add_builtin_double("ndoe", cm->proj[k].nDOE);
	add_builtin_double("age", cm->proj[k].age);
	add_builtin_double("sal", cm->proj[k].sal);
}

static Val interpret_code(const CurrentMember *cm, int k, const char *code,
		TypeKind return_type)
{
	if (!cm || !code) {
		return (Val){0};
	}
	add_builtin_vars(cm, k);
	const Database *db = get_database();
	assert(db);
	unsigned years = cm->proj[k].DOC->year - cm->proj[0].DOC->year;
	Interpreter *i = new_interpreter(code, db, years, cm->id, return_type);
	Val val = interpret(i);
	interpreter_free(i);
	return val;
}

void setassumptions(void)
{
	/* this needs updating when we have a UITY!!! */
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	char temp[BUFSIZ];
	char *year, *month, *day;
	year = month = day = 0;

	snprintf(temp, sizeof(temp), "%s", get_var(UI_DOC, VAR_FIXED, ht));
	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	ass.DOC = newDate(0, atoi(year), atoi(month), atoi(day));
	ass.DR = atof(get_var(UI_DR, VAR_FIXED, ht));
	ass.DR113 = atof(get_var(UI_DR113, VAR_FIXED, ht));
	ass.agecorr = atoi(get_var(UI_AGECORR, VAR_FIXED, ht));
	ass.infl = atof(get_var(UI_INFL, VAR_FIXED, ht));
	ass.ss = get_var(UI_SS, VAR_INTERPRETER, ht);
	ass.nra = get_var(UI_NRA, VAR_INTERPRETER, ht);
	ass.wxdef = get_var(UI_TURNOVER, VAR_INTERPRETER, ht);
	ass.retx = get_var(UI_RETX, VAR_INTERPRETER, ht);
	ass.calc_a = get_var(UI_CONTRA, VAR_INTERPRETER, ht);
	ass.calc_c = 0;
	ass.calc_dth = 0;

	// Assumptions that usually won't change from year to year
	ass.incrSalk0 = 0; // determine whether sal gets increased at k = 0
	ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1
	ass.TRM_PercDef = atof(get_var(UI_TRM_PERCDEF, VAR_FIXED, ht));

	set_methodology(ht);
}

static const char *get_var(unsigned ui, unsigned var_type,
		Hashtable ht[static 1])
{
	const char *s = 0;
	s = ht_get(get_ui_key(ui, var_type), ht);	
	return s;
}

static void set_methodology(Hashtable ht[static 1])
{
	if (atoi(get_var(COMBO_STANDARD, VAR_COMBO, ht))) ass.method += mIAS;
	if (PAR115 == atoi(get_var(COMBO_ASSETS, VAR_COMBO, ht)))
		ass.method += mPAR115;
	if (RES == atoi(get_var(COMBO_ASSETS, VAR_COMBO, ht)))
		ass.method += mRES;
	if (TUC == atoi(get_var(COMBO_ASSETS, VAR_COMBO, ht)))
		ass.method += mTUC;
	if (atoi(get_var(COMBO_MAXPUCTUC, VAR_COMBO, ht)))
		ass.method += mmaxPUCTUC;
	if (atoi(get_var(COMBO_MAXERCONTR, VAR_COMBO, ht)))
		ass.method += mmaxERContr;
	if (atoi(get_var(COMBO_EVALDTH, VAR_COMBO, ht))) ass.method += mDTH;

	if (ass.method & mIAS)
		ass.taxes = 0.0886 + 0.044;
	else
		ass.taxes = 0;

}

void set_tariffs(const CurrentMember cm[static 1])
{
	unsigned ltins = 0;
	unsigned ltterm = 0;
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	const char *s = get_var(UI_ADMINCOST, UI_INT, ht);
	tff.admincost = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_var(UI_COSTRES, UI_INT, ht);
	tff.costRES = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_var(UI_COSTKO, UI_INT, ht);
	tff.costKO = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_var(UI_WD, UI_INT, ht);
	tff.WDDTH = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	tff.MIXEDPS = 1;
	s = get_var(UI_PREPOST, UI_INT, ht);
	tff.prepost = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_var(UI_TERM, UI_INT, ht);
	tff.term = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	ltins = 0; // TODO
	ltterm = 0; // TODO
	for (int l = 0; l < EREE_AMOUNT; l++) {
		for (int j = 0; j < MAXGEN; j++) {
			tff.ltINS[l][j].lt = ltins;
			tff.ltAfterTRM[l][j].lt = ltterm;
			tff.ltINS[l][j].i = cm->TAUX[l][j];
			tff.ltAfterTRM[l][j].i = cm->TAUX[l][j];
		}
		tff.ltProlong[l].lt = tff.ltINS[l][0].lt;
		tff.ltProlongAfterTRM[l].lt = tff.ltAfterTRM[l][0].lt;
		tff.ltProlong[l].i = tff.ltINS[l][MAXGEN-1].i;
		tff.ltProlongAfterTRM[l].i = tff.ltAfterTRM[l][MAXGEN-1].i;
	}
}

double salaryscale(const CurrentMember cm[static 1], int k)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	const char *s = get_var(UI_SS, UI_INT, ht);
	return ass.infl + interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcA(const CurrentMember cm[static 1], int k)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	const char *s = get_var(UI_CONTRA, UI_INT, ht);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcC(const CurrentMember cm[static 1], int k)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	const char *s = get_var(UI_CONTRC, UI_INT, ht);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcDTH(const CurrentMember cm[static 1], int k)
{
	return gen_sum(cm->proj[k].gens, ER, CAPDTH, PUC);
}

double NRA(const CurrentMember cm[static 1], int k)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	const char *s = get_var(UI_NRA, UI_INT, ht);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double wxdef(const CurrentMember cm[static 1], int k)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	const char *s = get_var(UI_TURNOVER, UI_INT, ht);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double retx(const CurrentMember cm[static 1], int k)
{
	Hashtable *ht = get_user_input(USER_INPUT_LY);
	const char *s = get_var(UI_RETX, UI_INT, ht);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}
