#ifndef ACT_FUNCS_H
#define ACT_FUNCS_H

#include <stddef.h>

#define EPS 0.00000001

unsigned lx(const char *name, size_t age)
	__attribute__ ((pure));
double npx(const char *table, double ageX, double ageXn, int corr) 
	__attribute__ ((const));
double nEx(const char *table, double i, double charge, double ageX,
		double ageXn, int corr)
	__attribute__ ((const));
double axn(const char *table, double i, double charge, unsigned prepost,
		unsigned term, double ageX, double ageXn, int corr)
	__attribute__ ((const));
double Ax1n(const char *table, double i, double charge, double ageX,
		double ageXn, int corr)
	__attribute__ ((const));
double IAx1n(const char *table, double i, double charge, double ageX,
		double ageXn, int corr)
	__attribute__ ((const));
double Iaxn(const char *table, double i, double charge, unsigned prepost,
		unsigned term, double ageX, double ageXn, int corr)
	__attribute__ ((const));
double CAP_UKMS_UKZT(double res, double prem, double deltacap,
		double age, double RA, double ac, double Ex, double ax)
	__attribute__ ((const));
double CAP_UKMT(double res, double prem, double capdth, double ac,
		double Ex, double ax, double axcost, double Ax1, double IAx1,
		double Iax, double cKO)
	__attribute__ ((const));
double CAP_MIXED(double res, double prem, double ac, double Ex,
		double ax, double axcost, double Ax1, double x10,
		double MIXEDPS, double cKO)
	__attribute__ ((const));

#endif
