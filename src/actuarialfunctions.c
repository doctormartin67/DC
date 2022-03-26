#include "helperfunctions.h"
#include "actuarialfunctions.h"
#include "errorexit.h"

/*
 * inline functions
 */
double npx(register unsigned lt, register double ageX, register double ageXn,
		register int corr) PURE ;
double nEx(register unsigned lt, register double i, register double charge,
		register double ageX, register double ageXn,
		register int corr) PURE ;
double axn(register unsigned lt, register double i, register double charge,
		register unsigned prepost, register unsigned term,
		register double ageX, register double ageXn,
		register int corr) PURE ;
double CAP_UKMS_UKZT(double res, double prem, double deltacap, double age,
		double RA, double ac, double Ex, double ax) CONST ;
double CAP_UKMT(double res, double prem, double capdth, double ac,
		double Ex, double ax, double axcost, double Ax1, double IAx1,
		double Iax, double cKO) CONST ;
double CAP_MIXED(double res, double prem, double ac, double Ex,
		double ax, double axcost, double Ax1, double x10,
		double MIXEDPS, double cKO) CONST ;

/* 
 * nAx = v^(1/2)*1Qx + v^(1+1/2)*1Px*1q_{x+1} + ...
 * + v^(n-1+1/2)*{n-1}_Px*1Q_{x+n-1}
 */
double Ax1n(register unsigned lt, register double i, register double charge,
		register double ageX, register double ageXn, register int corr)
{
	register int k = 0;
	register int payments = 0;
	register double im = 0.0;
	register double v = 0.0;
	register double value = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		im = (1 + i)/(1 + charge) - 1;
		v = 1/(1 + im);
		payments = ageXn - ageX + EPS;

		while (payments--) {
			value += pow(v, k + 1.0/2)
				* npx(lt, ageX, ageX + k, corr)
				* (1 - npx(lt, ageX + k, ageX + k + 1, corr));
			k++;
		}
		value += pow(v, k + 1.0/2)
			* npx(lt, ageX, ageX + k, corr)
			* (1 - npx(lt, ageX + k, ageXn, corr));

		return value;
	}
}

// IAx1n = sum^{n-1}_k=1:k*1A_{x+k}*kEx
double IAx1n(register unsigned lt, register double i, register double charge,
		register double ageX, register double ageXn, register int corr)
{
	register int k = 1;
	register int payments = 0;
	register double value = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		payments = ageXn - ageX + EPS;
		while (payments--) {
			value += k * Ax1n(lt, i, charge, ageX + k - 1,
					ageX + k, corr)
				* nEx(lt, i, charge, ageX, ageX + k - 1, corr);
			k++;
		}
		value += k * Ax1n(lt, i, charge, ageX + k - 1,
				ageXn, corr)
			* nEx(lt, i, charge, ageX, ageXn, corr);

		return value;
	}
}

/* 
 * This function hasn't been completed because I don't think I need it.
 * At the moment it only works for term = 1
 */
double Iaxn(register unsigned lt, register double i, register double charge,
		register unsigned prepost, register unsigned term,
		register double ageX, register double ageXn, register int corr)
{
	register int k = 1;
	register int payments = 0;
	register double ageXk = 0.0;
	register double value = 0.0;

	if (ageX > ageXn + EPS) {
		return 0;
	} else {
		ageXk = ageX + (double)prepost/term;
		payments = (ageXn - ageX) * term + EPS;
		while (payments--) {
			value += k++ * nEx(lt, i, charge, ageX, ageXk, corr);
			ageXk += 1.0/term;
		}
		ageXk -= 1.0/term * prepost;
		value /= term;
		value += (ageXn - ageXk) * k
			* nEx(lt, i, charge, ageX,
					((int)(ageXn*term + EPS))/term
					+ term*prepost, corr);

		return value;
	}
}
