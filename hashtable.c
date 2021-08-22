#include "hashtable.h"

List *lookup(const char *t, const char *value, Hashtable *ht)
{
	List *pht = 0;
	register unsigned long hashval;
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
		pht = (List *)malloc(sizeof(*pht));
		if (0 == pht || 0 == (pht->key = strdup(key)))
			errExit("[%s] [malloc|strdup] returned NULL\n", 
					__func__);
		pht->next = ht->list[hashval];
		ht->list[hashval] = pht;
	} else {
		free(pht->value);
	}

	if (0 == (pht->value = strdup(value)))
		errExit("[%s] [strdup] returned NULL\n", __func__);

	return pht;
}

Hashtable *newHashtable(unsigned long n, unsigned casesens)
{
	Hashtable *ht;

	ht = jalloc(1, sizeof(Hashtable));
	ht->list = jalloc(n, sizeof(List *));
	ht->hashsize = n;
	ht->casesens = casesens;
	return ht;
}

void freeHashtable(Hashtable *ht)
{
	if (0 == ht) return;

	for (unsigned int i = 0; i < ht->hashsize; i++)
		freeList(ht->list[i]);

	free(ht->list);
	free(ht);
}

void freeList(List *l)
{
	if (0 != l) {
		free(l->key);
		free(l->value);
		freeList(l->next);	
		free(l);
	}
}
