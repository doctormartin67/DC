#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libraryheader.h"
#include "lifetables.h"

#define PATH "/home/doctormartin67/Projects/work/tables/tables/" //needs updating!!
enum {TOTALTABLES = 32, MAXAGE = 130, LENGTHLINE = 64};

static const char *lifetables[6] =
{"LXMR", "LXFR", "LXMK", "LXFK", "LXFK'", "Lxnihil"};

static unsigned short ltcnt; // This is updated in makeLifeTable
static int ltlist[TOTALTABLES][MAXAGE]; // each row is a life table

static void makeLifeTable(const char *, int *);

int lx(unsigned int ltindex, int age) {

    if (age > MAXAGE)
	return 0;
    else {
	if (ltlist[ltindex][0] == 0)
	    makeLifeTable(lifetables[ltindex], ltlist[ltindex]);
	return ltlist[ltindex][age];
    }
}

static void makeLifeTable(const char *name, int *clt) { //clt = current life table
    char line[LENGTHLINE];
    FILE *lt;
    char value[LENGTHLINE];
    char *vp = value;
    char *lp = line;
    char path[128]; 
    snprintf(path, sizeof(path), "%s", PATH);

    if ((lt = fopen(strcat(path, name), "r")) == NULL) {
	fprintf(stderr, "In function makeLifeTable: can't open %s\n", name);
	exit(1);
    }
    while((fgets(line, LENGTHLINE, lt))) {
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
