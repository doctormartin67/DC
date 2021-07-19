#include "libraryheader.h"
#include "validation.h"

void initValidator(Validator *val)
{
    for (int i = 0; i < MAXMSG; i++)
    {
	strcpy(val->MsgWarn[i], "");
	strcpy(val->MsgErr[i], "");
    }
    val->Warncnt = 0;
    val->Errcnt = 0;
    val->status = OK;
}

void updateValidation(Validator *val, Status status, const char *format, ...)
{
    char s[BUFSIZ];
    va_list argptr;
    va_start(argptr, format);

    vsnprintf(s, sizeof(s), format, argptr);
    va_end(argptr);

    if (strlen(s) > MAXMSGSIZE - 1)
	errExit("[%s] error message too large:\n[%s]\n", __func__, s);

    val->status = (val->status > status ? val->status : status);
    if (status == WARNING && val->Warncnt < MAXMSG)
	strcpy(val->MsgWarn[val->Warncnt++], s);
    else if (status == ERROR && val->Errcnt < MAXMSG)
	strcpy(val->MsgErr[val->Errcnt++], s);
    else
	printf("[%s] more error/warnings than were shown to the user\n", __func__);
}

void setMsgbuf(char buf[], Validator *val)
{
    unsigned int cnt = 0;
    char *s[MAXMSG];
    int maxbuf = 0;

    strcpy(buf, "");
    if (val->status == WARNING)
    {
	cnt = val->Warncnt;
	for (unsigned int i = 0; i < cnt; i++)
	    s[i] = val->MsgWarn[i];
    }
    else if (val->status == ERROR)
    {
	cnt = val->Errcnt;
	for (unsigned int i = 0; i < cnt; i++)
	    s[i] = val->MsgErr[i];
    }
    else
	return;

    for (unsigned int i = 0; i < cnt; i++)
    {
	maxbuf += strlen(s[i]);
	if (maxbuf > MAXMSG * MAXMSGSIZE - 2) /* 2 for '\0' and '\n' */
	    break;
	strcat(buf, s[i]);
	strcat(buf, "\n");
    }
}
