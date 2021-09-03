#ifndef HASHTABLE
#define HASHTABLE

struct linked_list {
	struct linked_list *next;
	char *key;
	char *value;
};

/* 
 * casesens = 1 for case sensitive, casesens = 0 for insensitive
 */
typedef struct {
	struct linked_list **list;
	unsigned long hashsize;
	unsigned casesens; 
} Hashtable;

/* 
 * search the hashtable for the key and return the found linked_list if value == 0.
 * if value is not 0, update the linked_list with the value
 */
struct linked_list *lookup(const char key[restrict static 1],
		const char *restrict value, Hashtable *restrict ht);
Hashtable *newHashtable(size_t n, unsigned casesens);
void freeHashtable(Hashtable *restrict ht);
void freeList(struct linked_list *restrict l);

#endif
