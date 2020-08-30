#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "actuarialfunctions.h"

static void setvars(int, char **);
static char *func;
static char *lt;
static double i, charge, ageX, ageXn;
static int corr;

int main(int argc, char *argv[]){
  if (argc < 6){
    printf("incorrect input\n");
    printf("syntax: Ax1n table i charge ageX ageXn corr\n");
    printf("If corr is omitted then 0 is taken by default.\n");
  }
  else{
    setvars(argc, argv);
    printf("%.8f\n", Ax1n(lt, i, charge, ageX, ageXn, corr));
  }
  return 0;
}

void setvars(int argc, char *argv[]){
  lt = *(argv+1);
  i = atof(*(argv+2));
  charge = atof(*(argv+3));
  ageX = atof(*(argv+4));
  ageXn = atof(*(argv+5));
  corr = (argc == 7 ? atoi(*(argv+6)) : 0);
}
