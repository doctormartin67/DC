#ifndef TREEERRORS
#define TREEERRORS

#include "interpreter.h"

typedef enum 
{
    NOERR, SCERR, ESERR, XERR, CERR, NOCERR, UNKRULEERR, NORULEERR, QUOTERR, 
    SEPERR, CONDERR, TOERR, ISERR, ELSERR
} TreeError;

extern const char *const strterrors[];

void setterrno(TreeError te);
TreeError getterrno(void);
const char *strterror(TreeError te);
int isvalidTree(const char *t);
int isvalidcond(CaseTree *ct);

#endif
