#ifndef ACT_FUNCS_H
#define ACT_FUNCS_H

#include <stddef.h>

unsigned lx(const char *name, size_t age) __attribute__ ((pure));
double npx(const char *table, double ageX, double ageXn, int corr) 
	__attribute__ ((const));
double nEx(const char *table, double i, double charge, double ageX,
		double ageXn, int corr) __attribute__ ((const));
double axn(const char *table, double i, double charge, unsigned prepost,
		unsigned term, double ageX, double ageXn, int corr)
	__attribute__ ((const));

#endif
