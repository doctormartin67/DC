#ifndef HASHTABLE
#define HASHTABLE

#define LHEADER "<<<<<"
#define RHEADER ">>>>>"

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

void ht_set(const char *t, const char *value, Hashtable *ht);
const char *ht_get(const char *t, Hashtable *ht);
void printHashtable(const Hashtable *ht);
void readHashtable(const char *fname, Hashtable *ht);
void writeHashtable(const char *fname, const Hashtable *ht);
Hashtable *newHashtable(size_t n, unsigned casesens);
void freeHashtable(Hashtable *restrict ht);
void freeList(struct linked_list *restrict l);

#endif
