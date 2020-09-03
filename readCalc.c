#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LENGTH BUFSIZ * 1000
#define END 1

int main(int argc, char **argv) {

  FILE *fp;
  char line[LENGTH];
  if ((fp = fopen(*(argv+1), "r")) == NULL) {
    perror(*(argv+1));
    exit(1);
  }
  int count = 0;
  int finalKey = 0;
  while (fgets(line, LENGTH, fp) != NULL && !finalKey) {
    if (strstr(line, "<c r=\"") == NULL ||
	strstr(line, "11\"") == NULL ||
	strstr(line, "<v>") == NULL)
      continue;
    printf("--------------------------------\n");
    printf("count = %d\n", ++count);
    printf("--------------------------------\n");
    char *begin = line;
    char *end;
    while ((begin = strstr(begin, "<v>")) != NULL) {
      begin += 3;
      end = strstr(begin, "</v>");
      *end = '\0';
      if (strcmp(begin, "ENTITY") == 0) {
	finalKey = END;
	break;
      }
      printf("begin = %s\n", begin);
      *end = '<';
    }
  }
  
  fclose(fp);
  return 0;
}


