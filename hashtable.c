#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libraryheader.h"
#include "hashtable.h"
#include "errorexit.h"

List *lookup(const char t[static restrict 1], const char *restrict value,
		Hashtable ht[static restrict 1])
{
	List *pht = 0;
	register unsigned long hashval = 0;
	char *s = 0, key[strlen(t) + 1];

	snprintf(key, sizeof(key), "%s", t);
	if (!ht->casesens) upper(key);

	/* 
	 * hash the key using the following: djb2 by Dan Bernstein 
	 * https://stackoverflow.com/questions/7666509/hash-function-for-string
	 */
	s = key;
	hashval = 5381;
	while (*s) hashval = ((hashval << 5) + hashval) + *s++;
	hashval %= ht->hashsize;

	for (pht = ht->list[hashval]; 0 != pht; pht = pht->next) {
		if (0 == strcmp(key, pht->key))
			break;
	}

	/* 
	 * if the user enters NULL as value, no new value is set and item is 
	 * returned
	 */
	if (0 == value) return pht;

	/* 
	 * if key has never been set, create new entry and set key and value 
	 * for new entry otherwise, remove current entry and set with new value
	 */
	if (0 == pht) {
		pht = jalloc(1, sizeof(*pht));
		if (0 == (pht->key = strdup(key)))
			errExit("[%s] [strdup] returned NULL\n", __func__);
		pht->next = ht->list[hashval];
		ht->list[hashval] = pht;
	} else {
		free(pht->value);
	}

	if (0 == (pht->value = strdup(value)))
		errExit("[%s] [strdup] returned NULL\n", __func__);

	return pht;
}

Hashtable *newHashtable(size_t n, unsigned casesens)
{
	Hashtable *ht = jalloc(1, sizeof(*ht));

	ht->list = jalloc(n, sizeof(*ht->list));
	ht->hashsize = n;
	ht->casesens = casesens;

	return ht;
}

void freeHashtable(Hashtable *restrict ht)
{
	if (0 == ht) return;

	for (unsigned i = 0; i < ht->hashsize; i++)
		freeList(ht->list[i]);

	free(ht->list);
	free(ht);
}

void freeList(List *restrict l)
{
	if (0 != l) {
		free(l->key);
		free(l->value);
		freeList(l->next);	
		free(l);
	}
}
