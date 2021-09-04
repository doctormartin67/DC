/* 
 * This is a program to test interpreter.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libraryheader.h"
#include "errorexit.h"
#include "treeerrors.h"

#define NORMAL "\x1B[0m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"

#define OPT_PRINT_TREE 01 /* '-p' to print the tree */

/* '-v' print status for all tests instead of only final status */
#define OPT_VERBOSE 02 

#define TESTS "tests:"

unsigned testTree(const CaseTree ct[static 1], FILE *fp, unsigned options);

int main(int argc, char *argv[])
{
	FILE *fp = 0;
	char **s = 0;
	long ftellval = 0;
	size_t bufsiz = 0;

	char *opt = argv[1];
	char *tests = 0;
	unsigned options = 0;
	unsigned treecnt = 0;
	unsigned index = 0;

	TreeError te = NOERR;
	CaseTree *ct = 0;

	if (argc < 2) die("syntax: interpret [-p|s] file1 [file2] [...]");

	if ('-' == *opt) {
		if (argc < 3)
			die("syntax: interpret [-p|s] file1 [file2] [...]");

		if (strlen(argv[1]) < 2)
			die("syntax: interpret [-p|s] file1 [file2] [...]");

		opt++;
		while (*opt) {
			switch(*opt) {
				case 'p': options |= OPT_PRINT_TREE;
					  break;
				case 'v': options |= OPT_VERBOSE;
					  break;
				default: die("Unknown option [-%c]", *opt);
			}
			opt++;
		}
	}

	treecnt = argc - (options ? 1 : 0) - 1;
	s = jalloc(treecnt, sizeof(*s));

	for (unsigned i = 0; i < treecnt; i++) {
		index = i + 1 + (options ? 1: 0);
		fp = fopen(argv[index], "r");

		if (0 == fp) die("No such file");

		if (0 != fseek(fp, 0L, SEEK_END))
			die("Unable to move to end of file");

		if (-1 == (ftellval = ftell(fp)))
			die("Unable to obtain file position");
		bufsiz = ftellval;

		s[i] = jalloc(bufsiz + 1, sizeof(*s[i]));

		if (0 != fseek(fp, 0L, SEEK_SET))
			die("Unable to move to beginning of file");

		if (fread(s[i], sizeof(char), bufsiz, fp) != bufsiz)
			die("Unable to read line");

		s[i][bufsiz] = '\0';
		tests = strstr(s[i], TESTS);
		if (tests > s[i])
			*(tests - 1) = '\0';

		s[i] = strclean(s[i]);
		ct = plantTree(s[i]);

		if (NOERR != (te = getterrno())) {
			printf("[%s] %s\n", argv[index], strterror(te));
			if (0 != fclose(fp))
				die("Unable to close file [%s]", argv[index]);

			setterrno(NOERR);
			continue;
		}

		if (options & OPT_PRINT_TREE) printTree(ct);

		if (0 == testTree(ct, fp, options))
			printf("[%s] %sALL TESTS PASSED%s\n", argv[index],
					GREEN, NORMAL);
		else
			printf("[%s] %sONE OR MORE TESTS FAILED%s\n",
					argv[index], RED, NORMAL);

		chopTree(ct);
		ct = 0;
		if (0 != fclose(fp))
			die("Unable to close file [%s]", argv[index]);
	}

	return 0;
}

unsigned testTree(const CaseTree ct[static 1], FILE *fp, unsigned options)
{
	unsigned status = 0;
	double dage = 0.0;
	char test[BUFSIZ];
	const void **rule_data = calloc(RULE_AMOUNT, sizeof(*rule_data));
	char *age, *reg, *cat, *value;
	const char *temp = 0;
	age = reg = cat = value = 0;

	if (0 != fseek(fp, 0L, SEEK_SET))
		die("Unable to move to beginning of file");

	while (0 != (temp = fgets(test, sizeof(test), fp)))
		if (0 != strstr(test, TESTS))
			break;

	if (0 == temp) {
		printf("No tests found\n");
		return status;
	}

	while (0 != fgets(test, sizeof(test), fp)) {
		age = strtok(test, ",");
		reg = strtok(0, ",");
		cat = strtok(0, "=");
		value = strtok(0, "\r\n\v\f"); /* end of line or file */

		if (0 == age || 0 == reg || 0 == cat || 0 == value )
			die("invalid test: %s", test);

		dage = atof(age);
		rule_data[AGE] = &dage;
		rule_data[REG] = reg;
		rule_data[CAT] = cat;

		if (interpret(ct, rule_data) == atof(value)) {
			if (options & OPT_VERBOSE) {
				printf("%s,%s,%s=%s ", age, reg, cat, value);
				printf("%sPASSED%s\n", GREEN, NORMAL);
			}
		} else {
			if (options & OPT_VERBOSE) {
				printf("%s,%s,%s=%s ", age, reg, cat, value);
				printf("%sFAILED%s\n", RED, NORMAL);
			}
			status = 1;
		}
	}

	free(rule_data);
	return status;
}
