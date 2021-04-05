#ifndef HASHTABLE
#define HASHTABLE

#define HASHSIZE 1048583 // I took 2^20 and then the next prime number

typedef struct list {
  struct list *next;
  char *key;
  char *value;
} List;

typedef struct {
    List **list;
    unsigned long hashsize;
} Hashtable;

// casesens determines whether the key is case sensitive.
// if it doesn't matter it is suggested to put it equal to
// 1 because case sensitive is faster
List *get(unsigned short casesens, char *key, Hashtable *);
List *set(unsigned short casesens, char *key, char *value, Hashtable *);
// This allocates memory for n Hashtable pointers
Hashtable *newHashtable(unsigned long n);

#endif
