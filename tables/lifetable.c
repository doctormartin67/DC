#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libraryheader.h"
#include "lifetables.h"

#define PATH "/home/doctormartin67/Projects/work/tables/tables/" //needs updating!!
enum {MAXAGE = 120};

static const char *lifetables[6] =
{"LXMR", "LXFR", "LXMK", "LXFK", "LXFK'", "Lxnihil"};

typedef struct {
    unsigned int ltindex;
    int lt[MAXAGE];
} LifeTable;

static unsigned short ltcnt; // This is updated in makeLifeTable
static LifeTable *ltlist;

static void append(unsigned int);
static void makeLifeTable(const char *, int *);

int lx(unsigned int ltindex, int age) {
    LifeTable *lt;
    lt = ltlist;

    if (age > MAXAGE)
	return 0;
    else {
	while (ltlist != NULL && lt - ltlist < ltcnt) {
	    if (strcmp(lifetables[lt->ltindex], lifetables[ltindex]) == 0)
		return lt->lt[age];	
	    lt++;
	}
	append(ltindex);
	return lx(ltindex, age);	
    }
    printf("ERROR in %s(%d, %d) (recursive function):", __func__, ltindex, age);
    printf("should never reach this point.\n");
    exit(1);
}

static void append(unsigned int lt) {
    if (ltcnt == 0) 
	ltlist = (LifeTable *)malloc(sizeof(LifeTable) * 2);
    else
	ltlist = (LifeTable *)realloc(ltlist, sizeof(LifeTable) * (ltcnt + 2));

    ltlist[ltcnt].ltindex = lt;
    makeLifeTable(lifetables[ltlist[ltcnt].ltindex], ltlist[ltcnt].lt);
}

static void makeLifeTable(const char *name, int *clt) { //clt = current life table
    char line[64];
    FILE *lt;
    char value[64];
    char *vp = value;
    char *lp = line;
    char path[128]; 
    snprintf(path, sizeof(path), "%s", PATH);

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
    ltcnt++;
    printf("amount of tables: %d\n", ltcnt);
}
