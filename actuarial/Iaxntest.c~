#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "actuarialfunctions.h"

static void setvars(int, char **);
static char *func;
static char *lt;
static double i, charge, ageX, ageXn;
static int corr, prepost, temp;

int main(int argc, char *argv[]){
  if (argc < 8){
    printf("incorrect input\n");
    printf("syntax: ./axn table i charge prepost temp ageX ageXn corr\n");
    printf("If corr is omitted then 0 is taken by default.\n");
  }
  else{
    setvars(argc, argv);
    printf("%.8f\n", axn(lt, i, charge, prepost, temp, ageX, ageXn, corr));
  }
  return 0;
}

void setvars(int argc, char *argv[]){
  lt = *(argv+1);
  i = atof(*(argv+2));
  charge = atof(*(argv+3));
  prepost = atoi(*(argv+4));
  temp = atoi(*(argv+5));
  ageX = atof(*(argv+6));
  ageXn = atof(*(argv+7));
  corr = (argc == 9 ? atoi(*(argv+8)) : 0);
}
