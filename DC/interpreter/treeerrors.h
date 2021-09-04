#ifndef TREEERRORS
#define TREEERRORS

#include "interpreter.h"

typedef enum {
    NOERR, SCERR, ESERR, XERR, CERR, NOCERR, UNKRULEERR, NORULEERR, QUOTERR, 
    SEPERR, CONDERR, TOERR, ISERR, ELSERR, NULLERR, TERR_AMOUNT
} TreeError;

extern const char *const strterrors[TERR_AMOUNT];

void setterrno(TreeError te);
TreeError getterrno(void);
const char *strterror(TreeError te);
unsigned isvalidTree(const char *t);
unsigned isvalidBranch(const struct casetree ct[static 1]);
unsigned isvalidLeaf(const char s[static 1]);

#endif
