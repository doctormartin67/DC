#include "libraryheader.h"
#include "inputerrors.h"

static const char *validMsg[] = {"[dd/mm/yyyy]", "^[+-]?[0-9]+\\.?[0-9]*$", 
    "^[+-]?[0-9][0-9]?$", "^[A-Z][A-Z]?[A-Z]?[1-9][0-9]*$"};

void setMsgErr(char msg[], const char *comment, const char s[], Err err)
{
    char temp[BUFSIZ];
    snprintf(temp, BUFSIZ, 
	    "%s%s: [%s] %10s %40s\n", msg, comment, s, "", validMsg[err]);
    strcpy(msg, temp);
}
