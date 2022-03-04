#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "helperfunctions.h"
#include "hashtable.h"
#include "errorexit.h"

/* 
 * search the hashtable for the key and return the found linked_list if value == 0.
 * if value is not 0, update the linked_list with the value
 */
static struct linked_list *lookup(const char t[restrict static 1],
		const char *restrict value, Hashtable ht[restrict static 1]);

static struct linked_list *lookup(const char t[restrict static 1],
		const char *restrict value, Hashtable ht[restrict static 1])
{
	struct linked_list *pht = 0;
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
			die("[strdup] returned NULL");
		pht->next = ht->list[hashval];
		ht->list[hashval] = pht;
	} else {
		free(pht->value);
	}

	if (0 == (pht->value = strdup(value)))
		die("[strdup] returned NULL");

	return pht;
}

void ht_set(const char t[restrict static 1],
		const char value[restrict static 1],
		Hashtable ht[restrict static 1])
{
	lookup(t, value, ht);
}

const char *ht_get(const char t[restrict static 1],
		Hashtable ht[restrict static 1])
{
	struct linked_list *l = lookup(t, 0, ht);

	if (l) return lookup(t, 0, ht)->value;
	else return 0;
}

void printHashtable(const Hashtable *restrict ht)
{
	if (0 == ht) {
		printf("Hashtable is NULL\n");
		return;
	}

	for (unsigned i = 0; i < ht->hashsize; i++) {
		if (0 == ht->list[i]) continue;
		for (struct linked_list *l = ht->list[i]; 0 != l; l = l->next){
			printf("%s%s%s\n", LHEADER, l->key, RHEADER);
			printf("%s\n", l->value);
		}
	}
}

/*
 * If a file is to be written that holds all the keys and corresponding values
 * of a Hashtable, then the file is a text file of the form
 * <<<<<KEY1>>>>>
 * value1
 * <<<<<KEY2>>>>>
 * value2
 * ...
 */
void readHashtable(const char fname[restrict static 1], Hashtable *restrict ht)
{
	FILE *fp = fopen(fname, "r");
	if (!fp) die("Unable to read file [%s]", fname);
	size_t len = BUFSIZ;
	char line[len];
	char value[len];
	char *key = 0;
	char *keyend = 0;
	char *pl = line;
	char *pv = value;
	long fppos = 0;

	if (0 == ht) {
		printf("Hashtable is NULL, no file read\n");
		return;
	}
	
	while (0 != fgets(line, len, fp)) {
		if (0 == (key = strinside(line, LHEADER, RHEADER))) continue;
		keyend = strstr(key, RHEADER);
		assert(0 != keyend);
		*keyend = '\0';
		key = strdup(key);

		while (0 != fgets(line, len, fp)) {
			if (strstr(line, LHEADER)) break;
			while (*pl && pv < value + len) *pv++ = *pl++;
			pl = line;
			if (-1 == (fppos = ftell(fp)))
				die("Unable to find position");
		}

		if (pv > value)
			*(pv - 1) = '\0';
		else
			*pv = '\0';
		
		ht_set(key, value, ht);

		pv = value;
		free(key);
		fseek(fp, fppos, SEEK_SET);
	}

	if (0 != fclose(fp)) die("Unable to close file");
}

void writeHashtable(const char *restrict fname, const Hashtable *restrict ht)
{
	FILE *fp = fopen(fname, "w");
	if (!fp) die("Unable to open/create file [%s]", fname);
	char buf[BUFSIZ];

	if (0 == ht) {
		printf("Hashtable is NULL, no file created\n");
		return;
	}

	for (unsigned i = 0; i < ht->hashsize; i++) {
		if (0 == ht->list[i]) continue;
		for (struct linked_list *l = ht->list[i]; 0 != l; l = l->next){
			snprintf(buf, sizeof(buf), "%s%s%s\n",
					LHEADER, l->key, RHEADER);
			if (fputs(buf, fp) == EOF) die("fputs returned EOF");
			snprintf(buf, sizeof(buf), "%s\n", l->value);
			if (fputs(buf, fp) == EOF) die("fputs returned EOF");
		}
	}

	if (0 != fclose(fp)) die("Unable to close file");
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

void freeList(struct linked_list *restrict l)
{
	if (0 != l) {
		free(l->key);
		free(l->value);
		freeList(l->next);	
		free(l);
	}
}
