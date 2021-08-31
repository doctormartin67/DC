#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libraryheader.h"
#include "errorexit.h"
#include "treeerrors.h"

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
	int options = 0;
	const int ptree = 01; /* print tree with '-p' option */
	const int surpress = 02; /* surpress individual tests '-s', only print final status */

	TreeError te = NOERR;
	int treecnt = 0;
	CaseTree *ct = 0;

	if (argc < 2)
	{
		printf("syntax: interpret [-p|s] file1 [file2] [...]\n");
		exit(0);
	}

	if ('-' == *opt)
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
		if (0 == fp) errExit("No such file\n");

		if (0 != fseek(fp, 0L, SEEK_END))
			errExit("Unable to move to end of file\n");

		if (-1 == (bufsiz = ftell(fp)))
			errExit("Unable to obtain file position\n");

		s[i] = jalloc(bufsiz + 1, sizeof(char));

		if (0 != fseek(fp, 0L, SEEK_SET))
			errExit("Unable to move to beginning of file\n");

		fread(s[i], sizeof(char), bufsiz, fp);
		/* error handling still to do here */

		s[i][bufsiz] = '\0';

		s[i] = strclean(s[i]);
		ct = buildTree(s[i]);

		/* If one tree has an error, print error message and continue with
		   next tree */
		if (NOERR != (te = getterrno()))
		{
			printf("[%s] %s\n", argv[index], strterror(te));
			if (0 != fclose(fp))
				errExit("Unable to close file [%s]\n", argv[index]);

			setterrno(NOERR);
			continue;
		}

		if (options & ptree)
			printTree(ct);

		if (0 == testTree(ct, fp, options & surpress))
			printf("[%s] %sALL TESTS PASSED%s\n", argv[index], GREEN, NORMAL);
		else
			printf("[%s] %sONE OR MORE TESTS FAILED%s\n", argv[index], RED, NORMAL);

		freeTree(ct);
		ct = 0;
		if (0 != fclose(fp))
			errExit("Unable to close file [%s]\n", argv[index]);
	}

	return 0;
}

int testTree(CaseTree *ct, FILE *fp, int surpress)
{
	int status = 0;
	char test[BUFSIZ]; /* no way test will be bigger than this */

	const void **rule_data;

	/* This needs improved !!!!!! */
	rule_data = calloc(3, sizeof(void *));
	char *age, *reg, *cat, *value;
	double dage;
	char *temp;

	if (0 != fseek(fp, 0L, SEEK_SET))
		errExit("Unable to move to beginning of file\n");

	while (0 != (temp = fgets(test, sizeof(test), fp)))
		if (0 != strstr(test, "tests:"))
			break;

	if (0 == temp)
	{
		printf("No tests found\n");
		return status;
	}

	while (0 != fgets(test, sizeof(test), fp))
	{
		age = strtok(test, ",");
		reg = strtok(0, ",");
		cat = strtok(0, "=");
		value = strtok(0, "\r\n\v\f"); /* end of line or file */

		if (0 == age || 0 == reg || 0 == cat || 0 == value )
			errExit("[%s] invalid test: %s\n", __func__, test);

		dage = atof(age);
		rule_data[AGE] = &dage;
		rule_data[REG] = reg;
		rule_data[CAT] = cat;

		if (interpret(ct, rule_data) == atof(value))
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
	free(rule_data);

	return status;
}
