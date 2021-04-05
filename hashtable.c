#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtable.h"
#include "libraryheader.h"

static inline unsigned long hash(char *, unsigned long);

// djb2 by Dan Bernstein https://stackoverflow.com/questions/7666509/hash-function-for-string
static inline unsigned long hash(char *s, unsigned long hashsize) {
    unsigned long hashval = 5381;

    while (*s)
	hashval = ((hashval << 5) + hashval) + *s++; /* hashval * 33 + c */

    return hashval % hashsize;
}

// if casesens is true then the key is case sensitive
List *get(unsigned short casesens, char *t, Hashtable *ht) {
    List *pht;
    unsigned long hashval; 
    char *key;
    key = (casesens ? t : upper(t));
    hashval = hash(key, ht->hashsize);
    for (pht = ht->list[hashval]; pht != NULL; pht = pht->next) {
	if (strcmp(key, pht->key) == 0) {
	    if (!casesens) free(key);
	    return pht;
	}
    }
    if (!casesens) free(key);
    return NULL;
}

List *set(unsigned short casesens, char *t, char *value, Hashtable *ht) {
    List *pht;
    unsigned long hashval;
    char *key;
    key = (casesens ? t : upper(t));
    if ((pht = get(casesens, key, ht)) == NULL) {
	pht = (List *) malloc(sizeof(*pht));
	if (pht == NULL || (pht->key = strdup(key)) == NULL) {
	    if (!casesens) free(key);
	    return NULL;
	}
	hashval = hash(key, ht->hashsize);
	pht->next = ht->list[hashval];
	ht->list[hashval] = pht;
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

Hashtable *newHashtable(unsigned long n) {
    Hashtable *ht;

    if ((ht = (Hashtable *)malloc(sizeof(Hashtable))) == NULL) {
	perror(__func__);
	exit(1);
    }
    else {
	ht->list = (List **)calloc(n, sizeof(List *));
	ht->hashsize = n;
	return ht;
    }
}
