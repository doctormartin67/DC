#include <stdio.h> // BUFSIZ
#include <string.h> // strchr()
#include <assert.h> // assert()
#include "common.h" // Map
#include "errorexit.h" // die()
#include "helperfunctions.h" // jalloc()

static Map life_tables;

enum {MAX_AGE = 512};

struct life_table {
	size_t size;
	unsigned lx[MAX_AGE];
};

static const struct life_table *make_life_table(const char *name)
{ 
	const char *lp = 0;
	char line[BUFSIZ];
	FILE *lt = 0;
	size_t age = 0;

	if (0 == (lt = fopen(name, "r"))) {
		die("can't open %s", name);
	}

	struct life_table *life_table = jalloc(1, sizeof(*life_table));

	while((fgets(line, BUFSIZ, lt))) {
		if (!(lp = strchr(line, ','))) {
			die("file '%s' has incorrect format", name);
		}
		if (age < MAX_AGE) {
			life_table->lx[age++] = atoi(++lp);
		}
	}

	fclose(lt);

	life_table->size = age;
	map_put(&life_tables, name, life_table);
	return life_table;
}


unsigned lx(const char *name, size_t age)
{
	assert(name);
	const struct life_table *life_table = map_get(&life_tables, name);
	if (!life_table) {
		life_table = make_life_table(name);
		assert(life_table);
		printf("life table '%s' created\n", name);
	}

	if (age > life_table->size - 1) {
		return 0;		
	} else {
		return life_table->lx[age];
	}
}
