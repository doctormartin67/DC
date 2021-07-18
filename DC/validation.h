#ifndef VALIDATION
#define VALIDATION

typedef enum {OK, ERROR, WARNING} Status;

/* these will be used as the indeces of validMsg array */
typedef enum {DATEERR, FLOATERR, AGECORRERR, CELLERR} Err;

/* The user will receive a maximum of 32 error messages of 256 length*/
enum {MAXMSG = 32, MAXMSGSIZE = 256}; 

typedef struct {
    char MsgWarn[MAXMSG][MAXMSGSIZE];
    char MsgErr[MAXMSG][MAXMSGSIZE];
    unsigned int Warncnt;
    unsigned int Errcnt;
    Status status;
} Validator;

extern const char *validMsg[];

void initValidator(Validator *val);
void updateValidation(Validator *val, Status status, const char *format, ...);
void setMsgbuf(char buf[], Validator *val);

#endif
