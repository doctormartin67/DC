#include "interpreter.h"

char *strclean(const char *);

int main(void) {
    char s[BUFSIZ];
    char *t = s;
    CaseTree *ct;

    while ((*t = getchar()) != EOF)
	t++;
    *t = '\0';

    ct = buildTree(s);
    printTree(ct);
    return 0;
}
