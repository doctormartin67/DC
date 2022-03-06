#ifndef VALIDATION
#define VALIDATION

typedef enum {OK, WARNING, ERROR} Status;

/* these will be used as the indeces of validMsg array */
typedef enum {DATEERR, FLOATERR, AGECORRERR, CELLERR, ERR_AMOUNT} Err;

/* The user will receive a maximum of 32 error messages of 256 length*/
enum {MAXMSG = 8, MAXMSGSIZE = 256}; 

typedef struct {
    char MsgWarn[MAXMSG][MAXMSGSIZE];
    char MsgErr[MAXMSG][MAXMSGSIZE];
    unsigned int Warncnt;
    unsigned int Errcnt;
    Status status;
} Validator;

extern const char *const validMsg[ERR_AMOUNT];

void updateValidation(Validator *val, Status status, const char *format, ...);
char *setMsgbuf(const Validator *val);

#endif
