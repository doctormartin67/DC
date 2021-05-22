#ifndef HASHTABLE
#define HASHTABLE

typedef struct list {
    struct list *next;
    char *key;
    char *value;
} List;

typedef struct {
    List **list;
    unsigned long hashsize;
    unsigned short casesens; // casesens = 1 for case sensitive, casesens = 0 for insensitive
} Hashtable;

// search the hashtable for the key and return the found List if value == NULL. if value is
// not NULL, update the List with the value
List *lookup(const char *key, const char *value, Hashtable *);
// This allocates memory for n Hashtable pointers
Hashtable *newHashtable(unsigned long n, unsigned short casesens);

#endif
