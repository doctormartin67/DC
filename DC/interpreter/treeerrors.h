#ifndef TREEERRORS
#define TREEERRORS

typedef enum 
{
    NOERR, SCERR, ESERR, XERR, CERR, NOCERR, UNKRULEERR, NORULEERR
} TreeError;

extern const char *strterrors[];

void setterrno(TreeError te);
TreeError getterrno(void);
const char *strterror(TreeError te);

#endif
