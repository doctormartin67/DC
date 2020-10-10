#ifndef HASHTABLE
#define HASHTABLE

#define HASHSIZE 32771 // I took BUFSIZ * 4 and then the next prime number

typedef struct hashtable {
  struct hashtable *next;
  char *key;
  char *value;
} Hashtable;

unsigned hash(char *);
Hashtable *get(char *key, Hashtable **);
Hashtable *set(char *key, char *value, Hashtable **);

#endif
