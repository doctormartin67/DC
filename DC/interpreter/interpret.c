#include "interpreter.h"

/* This is a program to test interpreter.c */

int main(int argc, char *argv[])
{
    FILE *fp;
    char **s;
    long bufsiz;
    int ptree = 0, treecnt;
    CaseTree *ct;

    if (argc < 2)
    {
	printf("syntax: interpret [-p] file1 [file2] [...]\n");
	exit(0);
    }

    if (argv[1][0] == '-') /* print tree option */
    {
	if (argc < 3)
	{
	    printf("syntax: interpret [-p] file1 [file2] [...]\n");
	    exit(0);
	}

	if (strlen(argv[1]) < 2)
	{
	    printf("syntax: interpret [-p] file1 [file2] [...]\n");
	    exit(1);
	}

	switch(argv[1][1])
	{
	    case 'p': ptree = 1;
		      break;

	    default: 
		      printf("Unknown option [-%d]\n", argv[1][1]);
		      exit(1);
	}
    }

    treecnt = argc - ptree - 1;
    s = calloc(treecnt, sizeof(char *));

    for (int i = 0; i < treecnt; i++)
    {
	fp = fopen(argv[i + 1 + ptree], "r");
	if (fp == NULL) errExit("No such file\n");

	if (fseek(fp, 0L, SEEK_END) != 0)
	    errExit("Unable to move to end of file\n");

	if ((bufsiz = ftell(fp)) == -1)
	    errExit("Unable to obtain file position\n");

	if ((s[i] = calloc(bufsiz + 1, sizeof(char))) == NULL)
	    errExit("calloc returned NULL\n");

	if (fseek(fp, 0L, SEEK_SET) != 0)
	    errExit("Unable to move to beginning of file\n");

	fread(s[i], sizeof(char), bufsiz, fp);
	/* error handling still to do here */

	s[i][bufsiz] = '\0';

	s[i] = strclean(s[i]);
	ct = buildTree(s[i]);

	if (ptree)
	    printTree(ct);
    }

    return 0;
}
