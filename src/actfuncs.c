#include <stdio.h> // BUFSIZ
#include <string.h> // strchr()
#include <assert.h> // assert()
#include <math.h> // floor()
#include "common.h" // Map
#include "errorexit.h" // die()
#include "helperfunctions.h" // jalloc()

#define EPS 0.0000001

static Map life_tables;

enum {MAX_AGE = 512};

struct life_table {
	size_t size;
	unsigned lx[MAX_AGE];
};

static const struct life_table *make_life_table(const char *name)
{ 
	const char *lp = 0;
	char line[BUFSIZ];
	FILE *lt = 0;
	size_t age = 0;

	if (0 == (lt = fopen(name, "r"))) {
		die("can't open %s", name);
	}

	struct life_table *life_table = jalloc(1, sizeof(*life_table));

	while((fgets(line, BUFSIZ, lt))) {
		if (!(lp = strchr(line, ','))) {
			die("file '%s' has incorrect format", name);
		}
		if (age < MAX_AGE) {
			life_table->lx[age++] = atoi(++lp);
		}
	}

	fclose(lt);

	life_table->size = age;
	map_put(&life_tables, name, life_table);
	return life_table;
}


unsigned lx(const char *name, size_t age)
{
	assert(name);
	const struct life_table *life_table = map_get(&life_tables, name);
	if (!life_table) {
		life_table = make_life_table(name);
		assert(life_table);
		printf("life table '%s' created\n", name);
	}

	if (age > life_table->size - 1) {
		return 0;		
	} else {
		return life_table->lx[age];
	}
}

double npx(const char *table, double ageX, double ageXn, int corr)
{
	ageX += corr;
	ageXn += corr;
	if (ageX == ageXn) {
		return 1.0;
	} else if (ageX > ageXn) {
		return 0.0;
	} else if (ageX < 0 || ageXn < 0) {
		return 0.0;
	}

	double ip1 = 0.0;
	double ip2 = 0.0;
	unsigned lxX = 0;
	unsigned lxX1 = 0;
	unsigned lxXn = 0;
	unsigned lxXn1 = 0;
	lxX = lx(table, (size_t)ageX);
	lxX1 = lx(table, (size_t)(ageX + 1));
	lxXn = lx(table, (size_t)ageXn);
	lxXn1 = lx(table, (size_t)(ageXn + 1));

	ip1 = lxX - (ageX - floor(ageX)) * (lxX - lxX1);
	ip2 = lxXn - (ageXn - floor(ageXn)) * (lxXn - lxXn1);

	if (0 == ip1) {
		assert(0 == ip2);
		return 0.0;
	} else {
		return ip2/ip1;
	}
}

double nEx(const char *table, double i, double charge, double ageX,
		double ageXn, int corr)
{
	double im = (1 + i)/(1 + charge) - 1;
	double n = ageXn - ageX;
	double vn = 1 / pow(1 + im, n);
	double nPx = npx(table, ageX, ageXn, corr);
	return vn * nPx;
}

/*
 * annuity immediate (prepost == 1) / due (prepost == 0). if 12 is not
 * divisible by 12 then return -1.0 to indicate incorrect input.
 */
double axn(const char *table, double i, double charge, unsigned prepost,
		unsigned term, double ageX, double ageXn, int corr)
{
	int payments = 0;
	double ageXk = 0.0;
	double value = 0.0;
	double termfrac = 0.0;

	if (ageX > ageXn + EPS) {
		value = 0;
	} else if (12 % term) {
		return -1.0;
	} else {
		termfrac = 1.0 / term;
		ageXk = ageX + (double)prepost/term;
		payments = (ageXn - ageX) * term + EPS;
		while (payments--) {
			value += nEx(table, i, charge, ageX, ageXk, corr);
			ageXk += termfrac;
		}
	}

	return value;
}

/* 
 * nAx = v^(1/2)*1Qx + v^(1+1/2)*1Px*1q_{x+1} + ...
 * + v^(n-1+1/2)*{n-1}_Px*1Q_{x+n-1}
 */
double Ax1n(const char *table, double i, double charge, double ageX,
		double ageXn, int corr)
{
	int k = 0;
	int payments = 0;
	double im = 0.0;
	double v = 0.0;
	double value = 0.0;
	double kPx = 0.0;
	double qx = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		im = (1 + i)/(1 + charge) - 1;
		v = 1/(1 + im);
		payments = ageXn - ageX + EPS;

		while (payments--) {
			kPx = npx(table, ageX, ageX + k, corr);
			qx = (1 - npx(table, ageX + k, ageX + k + 1, corr));
			value += pow(v, k + 1.0/2) * kPx * qx;
			k++;
		}
		kPx = npx(table, ageX, ageX + k, corr);
		qx = (1 - npx(table, ageX + k, ageXn, corr));
		value += pow(v, k + 1.0/2) * kPx * qx;

		return value;
	}
}

// IAx1n = sum^{n-1}_k=1:k*1A_{x+k}*kEx
double IAx1n(const char *table, double i, double charge, double ageX,
		double ageXn, int corr)
{
	int k = 1;
	int payments = 0;
	double value = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		payments = ageXn - ageX + EPS;
		while (payments--) {
			value += k * Ax1n(table, i, charge, ageX + k - 1,
					ageX + k, corr)
				* nEx(table, i, charge, ageX, ageX + k - 1, corr);
			k++;
		}
		value += k * Ax1n(table, i, charge, ageX + k - 1,
				ageXn, corr)
			* nEx(table, i, charge, ageX, ageXn, corr);

		return value;
	}
}

/* 
 * This function hasn't been completed because I don't think I need it.
 * At the moment it only works for term = 1
 */
double Iaxn(const char *table, double i, double charge, unsigned prepost,
		unsigned term, double ageX, double ageXn, int corr)
{
	int k = 1;
	int payments = 0;
	double ageXk = 0.0;
	double value = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		ageXk = ageX + (double)prepost/term;
		payments = (ageXn - ageX) * term + EPS;
		while (payments--) {
			value += k++ * nEx(table, i, charge, ageX, ageXk, corr);
			ageXk += 1.0/term;
		}
		ageXk -= 1.0/term * prepost;
		value /= term;
		value += (ageXn - ageXk) * k
			* nEx(table, i, charge, ageX,
					((int)(ageXn*term + EPS))/term
					+ term*prepost, corr);

		return value;
	}
}
