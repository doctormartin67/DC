#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtable.h"
#include "libraryheader.h"

// djb2 by Dan Bernstein https://stackoverflow.com/questions/7666509/hash-function-for-string
unsigned long hash(char *s) {
	unsigned long hashval = 5381;
	int c;

	while (c = *s++)
		hashval = ((hashval << 5) + hashval) + c; /* hashval * 33 + c */

	return hashval % HASHSIZE;
}

// if casesens is true then the key is case sensitive
Hashtable *get(unsigned short casesens, char *t, Hashtable **list) {
	static unsigned long colcnt;
	Hashtable *pht;
	char *key;
	unsigned long hashval;
	key = (casesens ? t : upper(t));
	for (pht = list[hash(key)]; pht != NULL; pht = pht->next) {
		if (strcmp(key, pht->key) == 0) {
			if (!casesens) free(key);
			return pht;
		}
		static unsigned short i = 1;
		if (++colcnt > 100000 * i) {
			printf("collision: %ld\n", colcnt);
			i++;
		}
	}
	if (!casesens) free(key);
	return NULL;
}

Hashtable *set(unsigned short casesens, char *t, char *value, Hashtable **list) {
	Hashtable *pht;
	unsigned hashval;
	char *key;
	key = (casesens ? t : upper(t));
	if ((pht = get(casesens, key, list)) == NULL) {
		pht = (Hashtable *) malloc(sizeof(*pht));
		if (pht == NULL || (pht->key = strdup(key)) == NULL) {
			if (!casesens) free(key);
			return NULL;
		}
		hashval = hash(key);
		pht->next = list[hashval];
		list[hashval] = pht;
	}
	else
		free((void *) pht->value);
	if ((pht->value = strdup(value)) == NULL) {
		if (!casesens) free(key);
		return NULL;
	}
	if (!casesens) free(key);
	return pht;
}
