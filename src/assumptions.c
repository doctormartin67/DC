#include <assert.h>
#include <math.h>
#include "type.h"
#include "assumptions.h"
#include "lifetables.h"
#include "interpret.h"
#include "resolve.h"

extern const Database *get_database(void);
enum {VAR_INTERPRETER, VAR_FIXED, VAR_COMBO};
static const char *get_var(unsigned ui, unsigned var_type);
static void set_methodology(void);

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
	char *temp = 0;
	char *year, *month, *day;
	year = month = day = 0;

	buf_printf(temp, "%s", get_var(UI_DOC, VAR_FIXED));
	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	ass.DOC = newDate(0, atoi(year), atoi(month), atoi(day));
	ass.DR = atof(get_var(UI_DR, VAR_FIXED));
	ass.DR113 = atof(get_var(UI_DR113, VAR_FIXED));
	ass.agecorr = atoi(get_var(UI_AGECORR, VAR_FIXED));
	ass.infl = atof(get_var(UI_INFL, VAR_FIXED));
	ass.ss = get_var(UI_SS, VAR_INTERPRETER);
	ass.nra = get_var(UI_NRA, VAR_INTERPRETER);
	ass.wxdef = get_var(UI_TURNOVER, VAR_INTERPRETER);
	ass.retx = get_var(UI_RETX, VAR_INTERPRETER);
	ass.calc_a = get_var(UI_CONTRA, VAR_INTERPRETER);
	ass.calc_c = 0;
	ass.calc_dth = 0;

	// Assumptions that usually won't change from year to year
	ass.incrSalk0 = 0; // determine whether sal gets increased at k = 0
	ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1
	ass.TRM_PercDef = atof(get_var(UI_TRM_PERCDEF, VAR_FIXED));

	set_methodology();
	buf_free(temp);
}

static const char *get_var(unsigned ui, unsigned var_type)
{
	Map *user_input = get_user_input();
	const char *s = map_get_str(user_input, get_ui_key(ui, var_type));	
	return s;
}

static void set_methodology(void)
{
	if (atoi(get_var(COMBO_STANDARD, VAR_COMBO))) ass.method += mIAS;
	if (PAR115 == atoi(get_var(COMBO_ASSETS, VAR_COMBO)))
		ass.method += mPAR115;
	if (RES == atoi(get_var(COMBO_ASSETS, VAR_COMBO)))
		ass.method += mRES;
	if (TUC == atoi(get_var(COMBO_ASSETS, VAR_COMBO)))
		ass.method += mTUC;
	if (atoi(get_var(COMBO_MAXPUCTUC, VAR_COMBO)))
		ass.method += mmaxPUCTUC;
	if (atoi(get_var(COMBO_MAXERCONTR, VAR_COMBO)))
		ass.method += mmaxERContr;
	if (atoi(get_var(COMBO_EVALDTH, VAR_COMBO))) ass.method += mDTH;

	if (ass.method & mIAS)
		ass.taxes = 0.0886 + 0.044;
	else
		ass.taxes = 0;

}

void set_tariffs(const CurrentMember cm[static 1])
{
	unsigned ltins = 0;
	unsigned ltterm = 0;
	const char *s = get_var(UI_ADMINCOST, UI_INT);
	tff.admincost = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_var(UI_COSTRES, UI_INT);
	tff.costRES = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_var(UI_COSTKO, UI_INT);
	tff.costKO = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_var(UI_WD, UI_INT);
	tff.WDDTH = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	tff.MIXEDPS = 1;
	s = get_var(UI_PREPOST, UI_INT);
	tff.prepost = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_var(UI_TERM, UI_INT);
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
	const char *s = get_var(UI_SS, UI_INT);
	return ass.infl + interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcA(const CurrentMember cm[static 1], int k)
{
	const char *s = get_var(UI_CONTRA, UI_INT);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcC(const CurrentMember cm[static 1], int k)
{
	const char *s = get_var(UI_CONTRC, UI_INT);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcDTH(const CurrentMember cm[static 1], int k)
{
	return gen_sum(cm->proj[k].gens, ER, CAPDTH, PUC);
}

double NRA(const CurrentMember cm[static 1], int k)
{
	const char *s = get_var(UI_NRA, UI_INT);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double wxdef(const CurrentMember cm[static 1], int k)
{
	const char *s = get_var(UI_TURNOVER, UI_INT);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double retx(const CurrentMember cm[static 1], int k)
{
	const char *s = get_var(UI_RETX, UI_INT);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}
