#include "lifetables.h"
#include "actuarialfunctions.h"

// npx is the chance for some of age ageX to live until the age of ageXn
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
