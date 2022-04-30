#include <assert.h>
#include <math.h>
#include "type.h"
#include "assumptions.h"
#include "lifetables.h"
#include "interpret.h"
#include "resolve.h"

extern const Database *get_database(void);
enum {VAR_INTERPRETER, VAR_FIXED, VAR_COMBO};
static void set_methodology(void);

// builtin vars
static const char *infl;
static const char *ndoe;
static const char *age;
static const char *sal;

static void add_builtin_vars(const CurrentMember *cm, int k)
{
	static unsigned interned;
	if (!interned) {
		infl = str_intern("infl");
		ndoe = str_intern("ndoe");
		age = str_intern("age");
		sal = str_intern("sal");
		interned = 1;
	}
	add_builtin_double(infl, ass.infl);
	add_builtin_double(ndoe, cm->proj[k].nDOE);
	add_builtin_double(age, cm->proj[k].age);
	add_builtin_double(sal, cm->proj[k].sal);
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
	Val val = interpret(code, db, years, cm->id, return_type);
	return val;
}

static const char *get_input(InputKind kind)
{
	UserInput *const *user_input = get_user_input();
	const char *s = user_input[kind]->input;
	return s;
}

void setassumptions(void)
{
	char *temp = 0;
	char *year, *month, *day;
	year = month = day = 0;

	buf_printf(temp, "%s", get_input(INPUT_DOC));
	day = strtok(temp, "/");
	month = strtok(0, "/");
	year = strtok(0, "");

	ass.DOC = newDate(0, atoi(year), atoi(month), atoi(day));
	ass.DR = atof(get_input(INPUT_DR));
	ass.DR113 = atof(get_input(INPUT_DR113));
	ass.agecorr = atoi(get_input(INPUT_AGECORR));
	ass.infl = atof(get_input(INPUT_INFL));
	ass.ss = get_input(INPUT_SS);
	ass.nra = get_input(INPUT_NRA);
	ass.wxdef = get_input(INPUT_TURNOVER);
	ass.retx = get_input(INPUT_RETX);
	ass.calc_a = get_input(INPUT_CONTRA);
	ass.calc_c = 0;
	ass.calc_dth = 0;

	// Assumptions that usually won't change from year to year
	ass.incrSalk0 = 0; // determine whether sal gets increased at k = 0
	ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1
	ass.TRM_PercDef = atof(get_input(INPUT_TRM_PERCDEF));

	set_methodology();
	buf_free(temp);
}

static void set_methodology(void)
{
	if (atoi(get_input(INPUT_STANDARD))) ass.method += mIAS;
	if (PAR115 == atoi(get_input(INPUT_ASSETS)))
		ass.method += mPAR115;
	if (RES == atoi(get_input(INPUT_ASSETS)))
		ass.method += mRES;
	if (TUC == atoi(get_input(INPUT_PUCTUC)))
		ass.method += mTUC;
	if (atoi(get_input(INPUT_MAXPUCTUC)))
		ass.method += mmaxPUCTUC;
	if (atoi(get_input(INPUT_MAXERCONTR)))
		ass.method += mmaxERContr;
	if (atoi(get_input(INPUT_EVALUATEDTH))) ass.method += mDTH;

	if (ass.method & mIAS)
		ass.taxes = 0.0886 + 0.044;
	else
		ass.taxes = 0;

}

void set_tariffs(const CurrentMember cm[static 1])
{
	unsigned ltins = 0;
	unsigned ltterm = 0;
	const char *s = get_input(INPUT_ADMINCOST);
	tff.admincost = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_input(INPUT_COSTRES);
	tff.costRES = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_input(INPUT_COSTKO);
	tff.costKO = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_input(INPUT_WD);
	tff.WDDTH = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	tff.MIXEDPS = 1;
	s = get_input(INPUT_PREPOST);
	tff.prepost = interpret_code(cm, 0, s, TYPE_DOUBLE).d;
	s = get_input(INPUT_TERM);
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
	const char *s = get_input(INPUT_SS);
	return ass.infl + interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcA(const CurrentMember cm[static 1], int k)
{
	const char *s = get_input(INPUT_CONTRA);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcC(const CurrentMember cm[static 1], int k)
{
	const char *s = get_input(INPUT_CONTRC);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double calcDTH(const CurrentMember cm[static 1], int k)
{
	return gen_sum(cm->proj[k].gens, ER, CAPDTH, PUC);
}

double NRA(const CurrentMember cm[static 1], int k)
{
	const char *s = get_input(INPUT_NRA);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double wxdef(const CurrentMember cm[static 1], int k)
{
	const char *s = get_input(INPUT_TURNOVER);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}

double retx(const CurrentMember cm[static 1], int k)
{
	const char *s = get_input(INPUT_RETX);
	return interpret_code(cm, k, s, TYPE_DOUBLE).d;
}
