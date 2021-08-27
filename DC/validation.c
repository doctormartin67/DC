#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "validation.h"
#include "errorexit.h"

void updateValidation(Validator *val, Status status, const char *format, ...)
{
    char s[BUFSIZ];
    char *t = s;
    va_list argptr;
    va_start(argptr, format);

    vsnprintf(s, sizeof(s), format, argptr);
    va_end(argptr);

    if (strlen(s) > MAXMSGSIZE - 1)
	errExit("[%s] error message too large:\n[%s]\n", __func__, s);

    val->status = (val->status > status ? val->status : status);
    if (status == WARNING && val->Warncnt < MAXMSG)
	snprintf(val->MsgWarn[val->Warncnt++], MAXMSGSIZE, "%s", t);
    else if (status == ERROR && val->Errcnt < MAXMSG)
	snprintf(val->MsgErr[val->Errcnt++], MAXMSGSIZE, "%s", t);
    else
	printf("[%s] more error/warnings than were shown to the user\n", __func__);
}

char *setMsgbuf(Validator *val)
{
    static char buf[BUFSIZ];
    unsigned cnt = 0;
    char *s[MAXMSG] = {0};
    size_t maxbuf = 0;

    *buf = '\0';
    if (val->status == WARNING)
    {
	cnt = val->Warncnt;
	for (unsigned i = 0; i < cnt; i++)
	    s[i] = val->MsgWarn[i];
    }
    else if (val->status == ERROR)
    {
	cnt = val->Errcnt;
	for (unsigned i = 0; i < cnt; i++)
	    s[i] = val->MsgErr[i];
    }
    else
	return buf;

    for (unsigned i = 0; i < cnt; i++)
    {
	maxbuf += snprintf(buf + maxbuf, sizeof(buf) - maxbuf, "%s\n", s[i]);
	if (maxbuf > sizeof(buf) - 2) /* 2 for '\0' and '\n' */
	    break;
    }

    return buf;
}
