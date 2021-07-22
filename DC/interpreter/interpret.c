#include "interpreter.h"

/* This is a program to test interpreter.c */

int main(int argc, char *argv[])
{
    FILE *fp;
    char s[BUFSIZ];
    char *t = s;
    int c, ptree = 0;
    CaseTree *ct;

    if (argc == 1)
    {
	while ((*t = getchar()) != EOF)
	    t++;
	*t = '\0';
    }
    else 
    {
	if (argv[1][0] == '-' && argv[1][1] == 'p') /* print tree option */
	{
	    fp = fopen(argv[2], "r");
	    ptree = 1;
	}
	else
	    fp = fopen(argv[1], "r");

	while ((c = fgetc(fp)) != EOF)
	    *t++ = (char )c;
	*t = '\0';
    }

    t = strclean(s);
    ct = buildTree(t);

    if (ptree)
	printTree(ct);
    
    return 0;
}
