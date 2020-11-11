#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libraryheader.h"

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("syntax: ./a.out 'string to convert to uppercase\n");
    exit(0);
  }

  char *temp;
  temp = *(argv+1);
  upper(temp);
  printf("%s\n", upper(temp));

  return 0;
}
