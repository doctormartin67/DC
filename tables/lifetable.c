#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <gmodule.h>

#define MAXLINE 15

//static GHashTable *lifetable;

void makeLifeTable(char *name) {
  char line[MAXLINE];
  int lineLength;
  FILE *lt;
  char key[MAXLINE];
  char value[MAXLINE];
  char *kp = key;
  char *vp = value;
  char *lp = line;
  
  if ((lt = fopen(name, "r")) == NULL) {
    fprintf(stderr, "can't open %s\n", name);
    exit(1);
  }
  while((fgets(line, MAXLINE, lt))) {
    lp = line;
    while ((*kp = *lp) != ',') {
      kp++;
      lp++;
    }
    *kp = '\0';
    lp++;
    while ((*vp = *lp)) {
      vp++;
      lp++;
    }
    *vp = '\0';
    kp = key;
    vp = value;
    printf("%s, %s", key, value);
  }
  
  fclose(lt);
}
