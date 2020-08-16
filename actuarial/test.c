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
  if (argc < 4){
    printf("incorrect input\n");
    printf("syntax: ./actfunc --function table i charge ageX ageXn corr\n");
    printf("If corr is omitted then 0 is taken by default.\n");
    printf("If --function is omitted then npx is taken by default.\n");
    printf("If i is omitted then 0 is taken by default.\n");
  }
  else{
    setvars(argc, argv);
    if(strcmp(func, "--nEx") == 0){
      printf("%.8f\n", nEx(lt, i, charge, ageX, ageXn, corr));
    }
    else if(strcmp(func, "--npx") == 0){
      printf("%.8f\n", npx(lt, ageX, ageXn, corr));
    }
    else{
      printf("function not found.\n");
      printf("options are:\n");
      printf("npx, nEx\n");
    }
  }
  return 0;
}

void setvars(int argc, char *argv[]){
  //check if there is a function argument, otherwise just take npx as default
  if(*(*(argv+1)) == '-' && *(*(argv+1)+1) == '-'){
    func = *(argv+1);
    lt = *(argv+2);
    double opt1 = atof(*(argv+3)); //optional argument;
    double opt2 = atof(*(argv+4)); //optional argument;
      
    if(opt1 < 1 && opt2 < 1){
      i = opt1;
      charge = opt2;
      ageX = atof(*(argv+5));
      ageXn = atof(*(argv+6));
      corr = (argc == 8 ? atoi(*(argv+7)) : 0);
    }
    else if(opt1 < 1){
      i = opt1;
      charge = 0;
      ageX = opt2;
      ageXn = atof(*(argv+5));
      corr = (argc == 7 ? atoi(*(argv+6)) : 0);
    }
    else{
      i = charge = 0;
      ageX = opt1;
      ageXn = atof(*(argv+4));
      corr = (argc == 6 ? atoi(*(argv+5)) : 0);
    }
  }
  //default is npx
  else{
    func = "--npx";
    lt = *(argv+1);
    i = 0;
    ageX = atof(*(argv+2));
    ageXn = atof(*(argv+3));
    corr = (argc == 5 ? atoi(*(argv+4)) : 0);
  }

}
