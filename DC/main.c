#include <studio.h>
#include "libraryheader.h"
#include "hashtable.h"
#include "DCProgram.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Syntax: run \"Excel file\" \"sheet name\"\n");
    exit(0);
  }
    
  XLfile xl;
  CurrentMember CM;
  
  setXLvals(&xl, argv[1]);
  
  return 0;
}
