#include <assert.h>
#include <math.h>
#include "type.h"
#include "assumptions.h"
#include "lifetables.h"
#include "interpret.h"
#include "resolve.h"
#include "errorjump.h"

extern const Database *get_database(void);

// builtin vars
static const char *infl;
static const char *ndoe;
static const char *age;
static const char *sal;

void init_builtin_vars(void)
{
	infl = str_intern("infl");
	ndoe = str_intern("ndoe");
	age = str_intern("age");
	sal = str_intern("sal");
	add_builtin_double(infl, ass.infl);
	add_builtin_double(ndoe, 0.0);
	add_builtin_double(age, 0.0);
	add_builtin_double(sal, 0.0);
}

static void add_builtin_vars(const CurrentMember *cm, int k)
{
	add_builtin_double(ndoe, cm->proj[k].nDOE);
	add_builtin_double(age, cm->proj[k].age);
	add_builtin_double(sal, cm->proj[k].sal);
}

static Val interpret_code(const CurrentMember *cm, int k, const char *code,
		const char *name, TypeKind return_type)
{
	if (!cm || !code) {
		return (Val){0};
	}
	add_builtin_vars(cm, k);
	const Database *db = get_database();
	assert(db);
	unsigned years = cm->proj[k].DOC->year - cm->proj[0].DOC->year;
	Val val = interpret(name, code, db, years, cm->id, return_type);
	assert(!error.is_error);
	return val;
}

static const char *get_input(InputKind kind)
{
	UserInput *const *user_input = get_user_input();
	return user_input[kind]->input;
}

static const char *get_name(InputKind kind)
{
	UserInput *const *user_input = get_user_input();
	return user_input[kind]->name;
}

static TypeKind get_return_type(InputKind kind)
{
	UserInput *const *user_input = get_user_input();
	return user_input[kind]->return_type;
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

	// Assumptions that usually won't change from year to year
	ass.incrSalk0 = 0; // determine whether sal gets increased at k = 0
	ass.incrSalk1 = 1; // determine whether sal gets increased at k = 1
	ass.TRM_PercDef = atof(get_input(INPUT_TRM_PERCDEF));

	set_methodology();
	init_builtin_vars();
	buf_free(temp);
}

static Val get_interpreter_val(const CurrentMember *cm, InputKind kind, int k)
{
	const char *code = get_input(kind);
	const char *name = get_name(kind);
	TypeKind return_type = get_return_type(kind);
	return interpret_code(cm, k, code, name, return_type);
}

void set_tariffs(const CurrentMember cm[static 1])
{
	const char *ltins = 0;
	const char *ltterm = 0;
	tff.admincost = get_interpreter_val(cm, INPUT_ADMINCOST, 0).d;
	tff.costRES = get_interpreter_val(cm, INPUT_COSTRES, 0).d;
	tff.costKO = get_interpreter_val(cm, INPUT_COSTKO, 0).d;
	tff.WDDTH = get_interpreter_val(cm, INPUT_WD, 0).d;
	tff.MIXEDPS = 1;
	tff.prepost = get_interpreter_val(cm, INPUT_PREPOST, 0).d;
	tff.term = get_interpreter_val(cm, INPUT_TERM, 0).d;
	ltins = get_interpreter_val(cm, INPUT_LTINS, 0).s;
	ltterm = get_interpreter_val(cm, INPUT_LTTERM, 0).s;
	for (int l = 0; l < EREE_AMOUNT; l++) {
		for (int j = 0; j < MAXGEN; j++) {
			tff.ltINS[l][j].table = ltins;
			tff.ltAfterTRM[l][j].table = ltterm;
			tff.ltINS[l][j].i = cm->TAUX[l][j];
			tff.ltAfterTRM[l][j].i = cm->TAUX[l][j];
		}
		tff.ltProlong[l].table = tff.ltINS[l][0].table;
		tff.ltProlongAfterTRM[l].table = tff.ltAfterTRM[l][0].table;
		tff.ltProlong[l].i = tff.ltINS[l][MAXGEN-1].i;
		tff.ltProlongAfterTRM[l].i = tff.ltAfterTRM[l][MAXGEN-1].i;
	}
}

double salaryscale(const CurrentMember cm[static 1], int k)
{
	double d = get_interpreter_val(cm, INPUT_SS, k).d;
	return ass.infl + d;
}

double calcA(const CurrentMember cm[static 1], int k)
{
	return get_interpreter_val(cm, INPUT_CONTRA, k).d;
}

double calcC(const CurrentMember cm[static 1], int k)
{
	return get_interpreter_val(cm, INPUT_CONTRC, k).d;
}

double calcDTH(const CurrentMember cm[static 1], int k)
{
	return gen_sum(cm->proj[k].gens, ER, CAPDTH, PUC);
}

double NRA(const CurrentMember cm[static 1], int k)
{
	return get_interpreter_val(cm, INPUT_NRA, k).d;
}

double wxdef(const CurrentMember cm[static 1], int k)
{
	return get_interpreter_val(cm, INPUT_TURNOVER, k).d;
}

double retx(const CurrentMember cm[static 1], int k)
{
	return get_interpreter_val(cm, INPUT_RETX, k).d;
}
