#include "hashtable.h"

List *lookup(const char *t, const char *value, Hashtable *ht)
{
    List *pht;
    register unsigned long hashval;
    char *s, key[strlen(t) + 1];

    strcpy(key, t);
    if (!ht->casesens)
	upper(key);

    // hash the key using the following:
    // djb2 by Dan Bernstein https://stackoverflow.com/questions/7666509/hash-function-for-string
    s = key;
    hashval = 5381;
    while (*s)
	hashval = ((hashval << 5) + hashval) + *s++; /* hashval * 33 + c */
    hashval %= ht->hashsize;


    // search item in list
    for (pht = ht->list[hashval]; pht != NULL; pht = pht->next)
    {
	if (strcmp(key, pht->key) == 0)
	    break;
    }

    // if the user enters NULL as value, no new value is set and item is returned
    if (value == NULL)
	return pht;

    // if key has never been set, create new entry and set key and value for new entry
    // otherwhise, remove current entry and set with new value
    if (pht == NULL)
    {
	pht = (List *)malloc(sizeof(*pht));
	if (pht == NULL || (pht->key = strdup(key)) == NULL)
	    errExit("[%s] [malloc|strdup] returned NULL\n", __func__);
	pht->next = ht->list[hashval];
	ht->list[hashval] = pht;
    }
    else
	free((void *)pht->value);
    if ((pht->value = strdup(value)) == NULL)
	errExit("[%s] [strdup] returned NULL\n", __func__);
    return pht;
}

Hashtable *newHashtable(unsigned long n, unsigned short casesens)
{
    Hashtable *ht;

    if ((ht = (Hashtable *)malloc(sizeof(Hashtable))) == NULL)
    {
	errExit("[%s] unable to create hashtable(%ld, %d)\n", __func__, n, casesens);
	return NULL; /* should never reach here!! */
    }
    else
    {
	if ((ht->list = (List **)calloc(n, sizeof(List *))) == NULL)
	    errExit("[%s] unable to create list\n", __func__);
	ht->hashsize = n;
	ht->casesens = casesens;
	return ht;
    }
}

void freeHashtable(Hashtable *ht)
{
    for (unsigned int i = 0; i < ht->hashsize; i++)
	    freeList(ht->list[i]);

    free(ht->list);
    free(ht);
}

void freeList(List *l)
{
    if (l != NULL)
    {
	free(l->key);
	free(l->value);
	freeList(l->next);	
	free(l);
    }
}
