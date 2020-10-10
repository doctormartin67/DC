#ifndef HASHTABLE
#define HASHTABLE

#define HASHSIZE 521 // I took 512 and then the next prime number

typedef struct hashtable {
  struct hashtable *next;
  char *key;
  char *value;
} Hashtable;

unsigned hash(char *);
Hashtable *get(char *key, Hashtable **);
Hashtable *set(char *key, char *value, Hashtable **);

#endif
