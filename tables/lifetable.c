#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libraryheader.h"
#include "lifetables.h"

#define MAXAGE 120
#define PATH "/home/doctormartin67/Projects/work/tables/tables/" //needs updating!!

typedef struct lifetable {
	char name[32];
	int lt[MAXAGE];
} LifeTable;

static unsigned short ltcnt; // This is updated in makeLifeTable
static LifeTable *ltlist;

static void append(char *);
static void makeLifeTable(char *, int *);

int lx(char *name, int age) {
	LifeTable *lt;
	lt = ltlist;

	if (age > MAXAGE)
		return 0;
	else {
		while (ltlist != NULL && lt - ltlist < ltcnt) {
			if (strcmp(lt->name, name) == 0)
				return lt->lt[age];	
			lt++;
		}
		append(name);
		lx(name, age);	
	}
}

static void append(char *name) {
	if (ltcnt == 0) 
		ltlist = (LifeTable *)malloc(sizeof(LifeTable) * 2);
	else
		ltlist = (LifeTable *)realloc(ltlist, sizeof(LifeTable) * (ltcnt + 2));

	snprintf(ltlist[ltcnt].name, sizeof(ltlist[ltcnt].name), "%s", name);
	makeLifeTable(ltlist[ltcnt].name, ltlist[ltcnt].lt);
}

static void makeLifeTable(char *name, int *clt) { //clt = current life table
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
