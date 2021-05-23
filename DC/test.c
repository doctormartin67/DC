#include "interpreter.h"

char *strclean(const char *);

int main(int argc, char *argv[]) {
    char s[BUFSIZ];
    char *t = s;
    CaseTree *ct;

    while ((*t = getchar()) != EOF)
	t++;
    *t = '\0';

    ct = buildTree(s);
    printf("x = %f\n", interpret(ct, atof(argv[1]), argv[2], argv[3]));
    return 0;
}
