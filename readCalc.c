#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libraryheader.h"

int main(int argc, char **argv) {

  printf("%s = %s\n", *(argv+3), cell(*(argv+3), *(argv+1), atoi(*(argv+2))));
  return 0;
}


