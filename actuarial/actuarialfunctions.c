#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lifetables.h"
#include "actuarialfunctions.h"

static const double eps = 0.0000001;
double npx(char *lt, double ageX, double ageXn, int corr){ //lt = life table
  /* generally the rule is npx = lx(ageXn)/lx(ageX) but because lx has
     an integer value as its input we need to interpolate both these
     values
  */
  double ip1; //interpolation value 1
  double ip2; //interpolation value 2

  ageX += corr;
  ageXn += corr;

  ip1 = lx(lt, ageX) - (ageX - (int)ageX) * (lx(lt, ageX) - lx(lt, ageX+1));
  ip2 = lx(lt, ageXn) - (ageXn - (int)ageXn) * (lx(lt, ageXn) - lx(lt, ageXn+1));
  
  return ip2/ip1;
}

double nEx(char *lt, double i, double charge, double ageX, double ageXn, int corr){
  double im = (1+i)/(1+charge) - 1;
  double n = ageXn - ageX;
  double vn = 1/pow(1 + im, n);
  double nPx = npx(lt, ageX, ageXn, corr);
  return vn * nPx;
}

double axn(char *lt, double i, double charge, int prepost, int term,
	   double ageX, double ageXn, int corr){
  if (12 % term != 0) {
    printf("An incorrect term was chosen, payments are usually monthly (term = 12)\n");
    printf("but can also be yearly for example (term = 1). term should be divisible\n");
    printf("by 12 but \nterm = %d.\n", term);
    exit(0);
  }
  else if (ageX > ageXn) {
    printf("warning: ageXn = %.6f < ageX = %.6f\n", ageXn, ageX);
    double temp = (double)(1 - prepost)/term;
    printf("prepost = %d and so axn = %.6f is taken\n", prepost, temp);
    return temp;
  }
  else {
    //    double im = pow(1 + i, 1.0/term) - 1; //=term interest rate (monthly: (1+i)^(1/12) - 1)
    // charge = pow(1 + charge, 1.0/term) - 1;
    double ageXk = ageX + (double)prepost/term; // current age in while loop
    int payments = (int)((ageXn - ageX) * term + eps);
    double value = 0; //return value;

    while (payments--) {
      value += nEx(lt, i, charge, ageX, ageXk, corr);
      ageXk += 1.0/term;
    }
    return value/term;
  }
}
