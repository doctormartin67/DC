#define _POSIX_C_SOURCE 1 /* for declaration of PATH_MAX */
#include <limits.h> /* for declaration of PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include "lifetables.h"
#include "errorexit.h"

#define PATH "/home/doctormartin67/Projects/work/tables/tables/" //needs updating!!
enum {TOTALTABLES = 32, MAXAGE = 130, LENGTHLINE = 64};

static const char *lifetables[6] =
{"LXMR", "LXFR", "LXMK", "LXFK", "LXFK'", "Lxnihil"};

static int ltlist[TOTALTABLES][MAXAGE];

static void makeLifeTable(const char *, int *);

int lx(register unsigned int ltindex, register int age)
{
    if (age > MAXAGE)
	return 0;
    else {
	if (ltlist[ltindex][0] == 0)
	    makeLifeTable(lifetables[ltindex], ltlist[ltindex]);
	return ltlist[ltindex][age];
    }
}

static void makeLifeTable(const char *name, int *clt)
{ 
    char line[LENGTHLINE];
    FILE *lt;
    char *lp = line;
    char path[PATH_MAX]; 
    snprintf(path, sizeof(path), "%s%s", PATH, name);

    if (0 == (lt = fopen(path, "r")))
	errExit("[%s] can't open %s\n", __func__, path);

    while((fgets(line, LENGTHLINE, lt))) {
	lp = line;
	while (',' != *lp && '\0' != *lp)
	    lp++;
	if ('\0' == *lp)
	    errExit("[%s] %s does not contain a ',', "
			    "so how does it separate age from value?\n", 
			    __func__, line);

	*clt++ = atoi(++lp);
    }

    fclose(lt);
}
