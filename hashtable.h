#define HASHSIZE 8192003 // I took BUFSIZ * 1000 and then the next prime number

typedef struct hashtable {
  struct hashtable *next;
  char *key;
  char *value;
} Hashtable;

Hashtable *list[HASHSIZE];
unsigned hash(char *);
Hashtable *get(char *key);
Hashtable *set(char *key, char *value);
