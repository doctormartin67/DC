#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "/home/doctormartin67/Projects/library/libraryheader.h"

#define MAXLINE 20
#define MAXAGE 120
#define HASHSIZE 101

static struct lifetable *list[HASHSIZE];
static unsigned hash(char *);
static struct lifetable *get(char *);
static struct lifetable *set(char *);
int lx(char *, int);
static void makeLifeTable(char *, int *);

struct lifetable {
  struct lifetable *next;
  char *name;
  int lt[MAXAGE];
  int count; //count the amount of times a lifetable gets set. more than once means big problems (collision)
};

int lx(char *name, int age) {
  struct lifetable *clt;
  if (get(name) == NULL)
    clt = set(name);
  return clt->lt[age];
}

static unsigned hash(char *s) {
  unsigned hashval;
  
  for (hashval = 0; *s != '\0'; s++)
    hashval = *s + 31 * hashval;
  return hashval % HASHSIZE;
}

static struct lifetable *get(char *s) {
  struct lifetable *plt;
  
  for (plt = list[hash(s)]; plt != NULL; plt = plt->next)
    if (strcmp(s, plt->name) == 0)
      return plt;
  return NULL;
}

static struct lifetable *set(char *name) {
  struct lifetable *plt;
  unsigned hashval;
  
  if ((plt = get(name)) == NULL) {
    plt = (struct lifetable *) malloc(sizeof(*plt));
    if (plt == NULL || (plt->name = strdup(name)) == NULL)
      return NULL;
    hashval = hash(name);
    plt->next = list[hashval];
    list[hashval] = plt;
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
  char line[MAXLINE];
  FILE *lt;
  char value[MAXLINE];
  char *vp = value;
  char *lp = line;
  
  if ((lt = fopen(name, "r")) == NULL) {
    fprintf(stderr, "In function makeLifeTable: can't open %s\n", name);
    exit(1);
  }
  while((fgets(line, MAXLINE, lt))) {
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
