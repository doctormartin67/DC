#include "interpreter.h"

#define NORMAL "\x1B[0m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"

/* This is a program to test interpreter.c */

const char *MsgTree[] = {"Tree passed all the tests", "No tests to check"};

int testTree(CaseTree *ct, FILE *fp, int surpress);

int main(int argc, char *argv[])
{
    FILE *fp;
    char **s;
    long bufsiz;

    /* options */
    char *opt = argv[1];
    short options = 0;
    const short ptree = 01; /* print tree with '-p' option */
    const short surpress = 02; /* surpress individual tests '-s', only print final status */

    int treecnt;
    CaseTree *ct;

    if (argc < 2)
    {
	printf("syntax: interpret [-p|s] file1 [file2] [...]\n");
	exit(0);
    }

    if (*opt == '-')
    {
	if (argc < 3)
	{
	    printf("syntax: interpret [-p|s] file1 [file2] [...]\n");
	    exit(0);
	}

	if (strlen(argv[1]) < 2)
	{
	    printf("syntax: interpret [-p|s] file1 [file2] [...]\n");
	    exit(1);
	}

	opt++;
	while (*opt)
	{
	    switch(*opt)
	    {
		case 'p': options |= ptree;
			  break;
		case 's': options |= surpress;
			  break;

		default: 
			  printf("Unknown option [-%c]\n", *opt);
			  exit(1);
	    }

	    opt++;
	}
    }

    treecnt = argc - (options ? 1 : 0) - 1;
    s = calloc(treecnt, sizeof(char *));

    for (int i = 0; i < treecnt; i++)
    {
	int index = i + 1 + (options ? 1: 0);
	fp = fopen(argv[index], "r");
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

	if (options & ptree)
	    printTree(ct);

	if (testTree(ct, fp, options & surpress) == 0)
	    printf("[%s] %sALL TESTS PASSED%s\n", argv[index], GREEN, NORMAL);
	else
	    printf("[%s] %sONE OR MORE TESTS FAILED%s\n", argv[index], RED, NORMAL);

	freeTree(ct);
	if (fclose(fp) != 0)
	    errExit("Unable to close file [%s]\n", argv[index]);
    }

    return 0;
}

int testTree(CaseTree *ct, FILE *fp, int surpress)
{
    int status = 0;
    char test[BUFSIZ]; /* no way test will be bigger than this */

    char *age, *reg, *cat, *value;
    char *temp;

    if (fseek(fp, 0L, SEEK_SET) != 0)
	errExit("Unable to move to beginning of file\n");

    while ((temp = fgets(test, sizeof(test), fp)) != NULL)
	if (strstr(test, "tests:") != NULL)
	    break;
    
    if (temp == NULL)
    {
	printf("No tests found\n");
	return status;
    }
    
    while (fgets(test, sizeof(test), fp) != NULL)
    {
	age = strtok(test, ",");
	reg = strtok(NULL, ",");
	cat = strtok(NULL, "=");
	value = strtok(NULL, "\r\n\v\f"); /* end of line or file */

	if (age == NULL || reg == NULL || cat == NULL || value == NULL)
	    errExit("[%s] invalid test: %s\n", __func__, test);

	if (interpret(ct, atof(age), reg, cat) == atof(value))
	{
	    if (!surpress)
	    {
		printf("%s,%s,%s=%s ", age, reg, cat, value);
		printf("%sPASSED%s\n", GREEN, NORMAL);
	    }
	}
	else
	{
	    if (!surpress)
	    {
		printf("%s,%s,%s=%s ", age, reg, cat, value);
		printf("%sFAILED%s\n", RED, NORMAL);
	    }
	    status = 1;
	}
    }

    return status;
}
