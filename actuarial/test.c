#include <stdio.h>
#include <stdlib.h>
#include "actuarialfunctions.h"

int main(int argc, char *argv[]){
  if (argc < 5){
    printf("incorrect input\n");
    printf("syntax: ./testnpx table ageX ageXn corr\n");
    return 0;
  }
  else{
    char *lt = *(argv+1);
    double ageX = atof(*(argv+2));
    double ageXn = atof(*(argv+3));
    int corr = atoi(*(argv+4));
    printf("%.8f\n", npx(lt, ageX, ageXn, corr));
    return 0;
  }
}
