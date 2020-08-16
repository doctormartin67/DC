#include <math.h>
#include "lifetables.h"
#include "actuarialfunctions.h"

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
