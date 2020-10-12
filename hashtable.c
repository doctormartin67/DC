#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtable.h"

unsigned hash(char *s) {
  unsigned hashval;
  
  for (hashval = 0; *s != '\0'; s++)
    hashval = *s + 31 * hashval;
  return hashval % HASHSIZE;
}

Hashtable *get(char *key, Hashtable **list) {
  Hashtable *pht;
  for (pht = list[hash(key)]; pht != NULL; pht = pht->next)
    if (strcmp(key, pht->key) == 0)
      return pht;
  return NULL;
}

Hashtable *set(char *key, char *value, Hashtable **list) {
  Hashtable *pht;
  unsigned hashval;
  if ((pht = get(key, list)) == NULL) {
    pht = (Hashtable *) malloc(sizeof(*pht));
    if (pht == NULL || (pht->key = strdup(key)) == NULL)
      return NULL;
    hashval = hash(key);
    pht->next = list[hashval];
    list[hashval] = pht;
  }
  else
    free((void *) pht->value);
  if ((pht->value = strdup(value)) == NULL)
    return NULL;
  return pht;
}
