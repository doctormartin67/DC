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

	/* 
	 * casesens = 1 for case sensitive, casesens = 0 for insensitive
	 */
	unsigned casesens; 
} Hashtable;

/* 
 * search the hashtable for the key and return the found List if value == NULL.
 * if value is not NULL, update the List with the value
 */
List *lookup(const char key[static restrict 1], const char *restrict value,
		Hashtable *restrict ht);

/* 
 * This allocates memory for n Hashtable pointers
 */
Hashtable *newHashtable(size_t n, unsigned casesens);
void freeHashtable(Hashtable *restrict ht);
void freeList(List *restrict l);

#endif
