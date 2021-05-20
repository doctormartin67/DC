#include <stdio.h>

char *strclean(const char *);

int main(void) {
    char s[BUFSIZ];
    char *t = s;

    while ((*t = getchar()) != EOF)
	t++;
    *t = '\0';
    printf("---------\n");
    printf("%s\n", s);
    printf("---------\n");
    printf("%s", strclean(s));

    return 0;
}
