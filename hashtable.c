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

Hashtable *get(char *t, Hashtable **list) {
  Hashtable *pht;
  char *key;
  key = upper(t);
  for (pht = list[hash(key)]; pht != NULL; pht = pht->next)
    if (strcmp(key, pht->key) == 0) {
      free(key);
      return pht;
    }
  free(key);
  return NULL;
}

Hashtable *set(char *t, char *value, Hashtable **list) {
  Hashtable *pht;
  unsigned hashval;
  char *key;
  key = upper(t);
  if ((pht = get(key, list)) == NULL) {
    pht = (Hashtable *) malloc(sizeof(*pht));
    if (pht == NULL || (pht->key = strdup(key)) == NULL) {
      free(key);
      return NULL;
    }
    hashval = hash(key);
    pht->next = list[hashval];
    list[hashval] = pht;
  }
  else
    free((void *) pht->value);
  if ((pht->value = strdup(value)) == NULL) {
    free(key);
    return NULL;
  }
  free(key);
  return pht;
}
