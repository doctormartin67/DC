#ifndef INPUTERRORS
#define INPUTERRORS

typedef enum {OK, ERROR, WARNING} Status;
typedef enum {DATEERR, FLOATERR, AGECORRERR, CELLERR} Err;

void setMsgErr(char msg[], const char *comment, const char s[], Err err);

#endif
