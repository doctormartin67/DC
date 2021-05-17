#include <stdio.h>

char *strclean(char *);

int main(void) {
    char s[BUFSIZ];
    char *t = s;

    while ((*t = getchar()) != EOF)
	t++;
    *t = '\0';
    printf("---------\n");
    printf("%s\n", s);
    printf("---------\n");
    printf("%s\n", strclean(s));

    return 0;
}
