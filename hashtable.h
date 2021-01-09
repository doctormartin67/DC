#ifndef HASHTABLE
#define HASHTABLE

#define HASHSIZE 1048583 // I took 2^20 and then the next prime number

typedef struct hashtable {
  struct hashtable *next;
  char *key;
  char *value;
} Hashtable;

unsigned long hash(char *);
// casesens determines whether the key is case sensitive.
// if it doesn't matter it is suggested to put it equal to
// 1 because case sensitive is faster
Hashtable *get(unsigned short casesens, char *key, Hashtable **);
Hashtable *set(unsigned short casesens, char *key, char *value, Hashtable **);

#endif
