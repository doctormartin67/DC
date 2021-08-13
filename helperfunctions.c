#include "libraryheader.h"

static void outputError(int useErr, int err, int flushStdout,
	const char *format, va_list ap);

char *trim(char *s)
{
    char *t;
    t = s;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
	s++;
    }
    t = s;
    while (*s)
	s++;
    s--;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
	*s-- = '\0';
    }
    return t;
}

void upper(char *s)
{
    while(*s) {
	*s = toupper(*s);
	s++;
    }
}

int isfloat(const char *s)
{
    while (isgarbage(*s))
	s++;    

    if(*s == '+' || *s == '-')
	s++;

    if (!isdigit(*s))
	return 0;

    while (isdigit(*s))
	s++;

    if (*s == '\0')
	return 1;
    else if (*s == '.')
	s++;
    else
	return 0;

    while (isdigit(*s))
	s++;

    if (*s == '\0')
	return 1;
    else
	return 0;
}

int isint(const char *s)
{
    while (isgarbage(*s))
	s++;    

    if(*s == '+' || *s == '-')
	s++;

    if (!isdigit(*s))
	return 0;

    while (isdigit(*s))
	s++;

    if (*s == '\0')
	return 1;
    else
	return 0;
}

int isgarbage(int c) 
{
    const char *s = GARBAGE;

    while (*s)
	if (c == *s++)
	    return 1;
    return 0;
}

// replace all occurences of string oldW with newW in s
char *replace(const char *s, const char *oldW, 	const char *newW)
{
    char *result; 
    int i, cnt = 0; 
    int newWlen = strlen(newW); 
    int oldWlen = strlen(oldW); 

    // Counting the number of times old word 
    // occur in the string 
    for (i = 0; s[i] != '\0'; i++)
    {
	if (strstr(&s[i], oldW) == &s[i])
	{
	    cnt++; 

	    // Jumping to index after the old word. 
	    i += oldWlen - 1; 
	} 
    } 

    // Making new string of enough length 
    result = (char *) malloc(i + cnt * (newWlen - oldWlen) + 1); 
    if (result == NULL) errExit("[%s] malloc returned NULL\n", __func__);

    i = 0; 
    while (*s)
    {
	// compare the substring with the result 
	if (strstr(s, oldW) == s)
	{
	    strcpy(&result[i], newW); 
	    i += newWlen; 
	    s += oldWlen; 
	} 
	else
	    result[i++] = *s++; 
    } 

    result[i] = '\0'; 
    return result; 
}

// Check if DIR exists
int DIRexists(const char *dname)
{
    DIR* dir = opendir(dname);
    if (dir) {
	/* Directory FILEexists. */
	return 1;
	closedir(dir);
    } else if (ENOENT == errno) {
	/* Directory does not exist. */
	return 0;
    } else {
	/* opendir() failed for some other reason. */
	return -1;
    }
}

/*
 * finds the first occurence of 'begin' and 'end' inside 's' and returns a
 * pointer to the string that lies between 'begin' and 'end'. Returns NULL if
 * neither were found inside 's'.
 */

char *strinside(const char *s, const char *begin, const char *end)
{
    char *pb; // pointer to begin in s
    char *pe; // pointer to end in s
    if ((pb = strstr(s, begin)) == NULL)
	return NULL;
    /* 
     * pe should start looking for end starting at begin (+1 just in case begin
     * and end are the same string, otherwise the strstr function would find 
     * the same string and pb would be equal to pe)
     */
    pe = pb + 1;
    if ((pe = strstr(pe, end)) == NULL)
	return NULL;

    /* move pointer to start of the value we want */
    pb += strlen(begin);

    return pb;
}

// MAKE SURE YOU INPUT DOUBLE AS ARGUMENTS OR YOU WILL HAVE UNDEFINED BEHAVIOUR!
double min(int argc, ...)
{
    va_list valist;
    double value;
    double temp;

    /* initialize valist for num number of arguments */
    va_start(valist, argc);

    /* access all the arguments assigned to valist */
    value = va_arg(valist, double);
    for (int i = 0; i < argc - 1; i++) {
	temp = va_arg(valist, double);
	value = (value < temp ? value : temp);
    }

    /* clean memory reserved for valist */
    va_end(valist);

    return value;
}

// MAKE SURE YOU INPUT DOUBLE AS ARGUMENTS OR YOU WILL HAVE UNDEFINED BEHAVIOUR!
double max(int argc, ...)
{
    va_list valist;
    double value;
    double temp;

    /* initialize valist for num number of arguments */
    va_start(valist, argc);

    /* access all the arguments assigned to valist */
    value = va_arg(valist, double);
    for (int i = 0; i < argc - 1; i++) {
	temp = va_arg(valist, double);
	value = (value > temp ? value : temp);
    }

    /* clean memory reserved for valist */
    va_end(valist);

    return value;
}

double sum(double a[], int length)
{
    double value = 0;
    for (int i = 0; i < length; i++)
	value += *a++;
    return value;
}

static void outputError(int useErr, int err, int flushStdout, 
	const char *format, va_list ap)
{
    char buf[BUFSIZ * 4], userMsg[BUFSIZ], errText[BUFSIZ];

    vsnprintf(userMsg, BUFSIZ, format, ap);

    if (useErr)
	snprintf(errText, sizeof(errText), " [%s]", strerror(err));
    else
	snprintf(errText, sizeof(errText), ":");

    snprintf(buf, sizeof(buf), "ERROR%s %s\n", errText, userMsg);

    if (flushStdout)
	fflush(stdout);
    fputs(buf, stderr);
    fflush(stderr);
}
void errExit(const char *format, ...)
{
    va_list arglist;

    va_start(arglist, format);
    outputError(1, errno, 1, format, arglist);
    va_end(arglist);

    exit(EXIT_FAILURE);
}

/* same as errExit but used when diagnosing Pthreads */
void errExitEN(int errnum, const char *format, ...)
{
    va_list arglist;

    va_start(arglist, format);
    outputError(1, errnum, 1, format, arglist);
    va_end(arglist);

    exit(EXIT_FAILURE);
}

/* xml functions */
/* returns xmlDocPtr or NULL if docname could not be found */
xmlDocPtr getxmlDoc(const char *docname)
{
    xmlDocPtr doc;
    if((doc = xmlParseFile(docname)) == NULL)
	fprintf(stderr, "[%s] Unable to parse file [%s]\n", __func__, docname);

    return doc;
}

xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    context = xmlXPathNewContext(doc);

    if (context == NULL)
	errExit("[%s] xmlXPathNewContext returned NULL\n", __func__);
    if (xmlXPathRegisterNs(context,  BAD_CAST NSPREFIX, BAD_CAST NSURI) != 0)
	errExit("[%s] Unable to register NS with prefix\n", __func__);

    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);

    if (result == NULL)
	errExit("[%s] xmlXPathEvalExpression returned NULL\n", __func__);
    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
	xmlXPathFreeObject(result);
	printf("No result\n");
	return NULL;
    }
    return result;
}

/* --- free memory function --- */

void freeXL(XLfile *xl)
{
    if (xl == NULL)
	return;

    /* free xml's */
    xmlFreeDoc(xl->workbook);
    xmlFreeDoc(xl->sharedStrings);
    for (unsigned int i = 0; i < xl->sheetcnt; i++)
    {
	xmlFreeDoc(xl->sheets[i]);
	xmlXPathFreeObject(xl->nodesets[i]);
    }
    free(xl->sheets);
    free(xl->nodesets);
    xmlXPathFreeObject(xl->nodesetss);

    /* free others */
    free(xl->sheetname);
    free(xl);
}

/* --- NON PORTABLE FUNCTION --- */

void createXLzip(const char *s)
{
    char t[strlen(s) + 1];
    char *pt = t;
    const char *prevpxls = s, *pxls = NULL, *ps = s;
    char dirname[strlen(s) + 1];
    char cmd[BUFSIZ];

    /* find final occurrence of ".xls", usually it should only occur once, but you never know */
    while ((prevpxls = strstr(prevpxls, ".xls")) != NULL)
	pxls = prevpxls++;

    if (pxls == NULL)
	errExit("[%s] [%s] not a valid excel file\n", __func__, s);

    while (ps < pxls)
	*pt++ = *ps++;
    *pt = '\0';
    strcpy(dirname, t);

    strcat(t, ".zip");
    
    /* remove previous zip file, if it exists */
    snprintf(cmd, sizeof(cmd), "%s%s%c", "rm -rf '", t, '\'');

    /* THIS IS NOT PORTABLE !!! */
    if (system(cmd) != 0)
	errExit("[%s] system command [%s] failed, are you on windows?\n", __func__, cmd);

    snprintf(cmd, sizeof(cmd), "%s%s%c%s%s%c", "cp '", s, '\'', " '", t, '\'');

    /* THIS IS NOT PORTABLE !!! */
    if (system(cmd) != 0)
	errExit("[%s] system command [%s] failed, are you on windows?\n", __func__, cmd);

    /* remove previous folder, if it exists */
    snprintf(cmd, sizeof(cmd), "%s%s%c", "rm -rf '", dirname, '\'');

    /* THIS IS NOT PORTABLE !!! */
    if (system(cmd) != 0)
	errExit("[%s] system command [%s] failed, are you on windows?\n", __func__, cmd);

    if (mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
	errExit("[%s] Error making directory [%s]\n", __func__, dirname);

    snprintf(cmd, sizeof(cmd), "%s%s%s%s%c", "unzip -q '", t, "' -d '", dirname, '\'');

    /* THIS IS NOT PORTABLE !!! */
    if (system(cmd) != 0)
	errExit("[%s] system command [%s] failed, are you on windows?\n", __func__, cmd);
}
