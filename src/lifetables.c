#define _POSIX_C_SOURCE 1 /* for declaration of PATH_MAX */
#include <limits.h> /* for declaration of PATH_MAX */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "lifetables.h"
#include "errorexit.h"
#include "common.h"

#define PATH "/tables/tables/"

static const char *const lifetables[LT_AMOUNT] = {
	[LXMR] = "LXMR",
	[LXFR] = "LXFR",
	[LXMK] = "LXMK",
	[LXFK] = "LXFK",
	[LXFKP] = "LXFK'",
	[LXNIHIL] = "Lxnihil"
};

unsigned long lx[LT_AMOUNT][MAXAGE];

static void makeLifeTable(const char *, unsigned long *);

void makeLifeTables(void)
{
	for (unsigned i = 0; i < LT_AMOUNT; i++) {
		makeLifeTable(lifetables[i], lx[i]);
	}
}

static void makeLifeTable(const char name[restrict static 1],
		unsigned long clt[static 1])
{ 
	char cwd[PATH_MAX];
	char *path = 0;
	char line[LENGTHLINE];
	FILE *lt = 0;
	char *lp = 0;
	
	if (!getcwd(cwd, sizeof(cwd))) {
		die("Unable to call getcwd");
	}
	buf_printf(path, "%s%s%s", cwd, PATH, name);

	if (0 == (lt = fopen(path, "r"))) die("can't open %s", path);

	while((fgets(line, LENGTHLINE, lt))) {
		lp = line;
		while (',' != *lp && '\0' != *lp) lp++;

		if ('\0' == *lp)
			die("%s does not contain a ',', so how does "
					"it separate age from value?", line);

		*clt++ = atol(++lp);
	}
	
	fclose(lt);
	buf_free(path);
}
