#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libraryheader.h"
#include "lifetables.h"

#define MAXAGE 120
#define HASHLTSIZE 101
#define PATH "/home/doctormartin67/Projects/work/tables/tables/"

static struct lifetable *list[HASHLTSIZE];
static unsigned hashlt(char *);
static struct lifetable *getlt(char *);
static struct lifetable *setlt(char *);
static void makeLifeTable(char *, int *);

struct lifetable {
  struct lifetable *next;
  char *name;
  int lt[MAXAGE];
  int count; //count the amount of times a lifetable gets set. more than once means big problems (collision)
};

int lx(char *name, int age) {
  if (age > MAXAGE)
    return 0;
  else {
    struct lifetable *clt;
    if ((clt = getlt(name)) == NULL)
      clt = setlt(name);
    return clt->lt[age];
  }
}

static unsigned hashlt(char *s) {
  unsigned hashltval;
  
  for (hashltval = 0; *s != '\0'; s++)
    hashltval = *s + 31 * hashltval;
  return hashltval % HASHLTSIZE;
}

static struct lifetable *getlt(char *s) {
  struct lifetable *plt;
  
  for (plt = list[hashlt(s)]; plt != NULL; plt = plt->next)
    if (strcmp(s, plt->name) == 0)
      return plt;
  return NULL;
}

static struct lifetable *setlt(char *name) {
  struct lifetable *plt;
  unsigned hashltval;
  
  if ((plt = getlt(name)) == NULL) {
    plt = (struct lifetable *) malloc(sizeof(*plt));
    if (plt == NULL || (plt->name = strdup(name)) == NULL)
      return NULL;
    hashltval = hashlt(name);
    plt->next = list[hashltval];
    list[hashltval] = plt;
    plt->count = 1;
  }
  else {
    free((void *) plt->lt);
    plt->count++;
    printf("BIG PROBLEM: COLLISION OCCURRED AT %s, count = %d\n", plt->name, plt->count);
  }
  makeLifeTable(name, plt->lt);
  if (plt->lt == NULL)
    return NULL;
  return plt;
}

static void makeLifeTable(char *name, int *clt) { //clt = current life table
  char line[BUFSIZ];
  FILE *lt;
  char value[BUFSIZ];
  char *vp = value;
  char *lp = line;
  char path[100] = "";
  strcat(path, PATH);
  
  if ((lt = fopen(strcat(path, name), "r")) == NULL) {
    fprintf(stderr, "In function makeLifeTable: can't open %s\n", name);
    exit(1);
  }
  while((fgets(line, BUFSIZ, lt))) {
    lp = line;
    while (*lp++ != ',') {
      ;
    }
    while ((*vp = *lp)) {
      vp++;
      lp++;
    }
    vp = trim(value);
    *clt++ = atoi(vp);
    vp = value;
  }

  fclose(lt);
}
