#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtable.h"
#include "libraryheader.h"

unsigned hash(char *s) {
	unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;
	return hashval % HASHSIZE;
}

// if casesens is true then the key is case sensitive
Hashtable *get(unsigned short casesens, char *t, Hashtable **list) {
	Hashtable *pht;
	char *key;
	key = (casesens ? t : upper(t));
	for (pht = list[hash(key)]; pht != NULL; pht = pht->next)
		if (strcmp(key, pht->key) == 0) {
			if (!casesens) free(key);
			return pht;
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
