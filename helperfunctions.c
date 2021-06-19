#include "libraryheader.h"

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

// replace all occurences of string oldW with newW in s
char *replace(const char *s, const char *oldW, 	const char *newW)
{
    char *result; 
    int i, cnt = 0; 
    int newWlen = strlen(newW); 
    int oldWlen = strlen(oldW); 

    // Counting the number of times old word 
    // occur in the string 
    for (i = 0; s[i] != '\0'; i++) { 
	if (strstr(&s[i], oldW) == &s[i]) { 
	    cnt++; 

	    // Jumping to index after the old word. 
	    i += oldWlen - 1; 
	} 
    } 

    // Making new string of enough length 
    result = (char *) malloc(i + cnt * (newWlen - oldWlen) + 1); 

    i = 0; 
    while (*s) { 
	// compare the substring with the result 
	if (strstr(s, oldW) == s) { 
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

// Check if file exists
int FILEexists(const char *fname)
{
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
	fclose(file);
	return 1;
    }
    return 0;
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

/* the xml files for excel are often in the form:
   <v>...<\v> and so we want to value where ...
   lies. This is a small function to retrieve the
   ... given begin (<v>) and end (<\v>). 
   REMEMBER TO FREE THE RETURN VALUE WHEN YOU ARE
   FINISHED WITH IT!
 */
char *strinside(const char *s, const char *begin, const char *end)
{
    char *pb; //pointer to begin in s
    char *pe; //pointer to end in s
    char *value; //malloc result that we will return
    int i, length; 
    if ((pb = strstr(s, begin)) == NULL) {
	return NULL;
    }
    /* pe should start looking for end starting at begin (+1 just in case begin and end are the
       same string, otherwise the strstr function would find the same string and pb would be equal
       to pe) */
    pe = pb + 1;
    if ((pe = strstr(pe, end)) == NULL) {
	return NULL;
    }

    // move pointer to start of the value we want
    pb += strlen(begin);

    /* pb is pointing at the character just after
       the last character of the value we need and
       so pe - pb is exactly the amount of characters
       of value, we then need one extra value for '\0'.
     */
    length = pe - pb + 1;
    value = (char *)calloc(length, sizeof(char));
    for (i = 0; i < length; i++) {
	value[i] = *pb++;
    }
    value[i-1] = '\0';
    return value;

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

void errExit(const char *func, const char *format, ...)
{
    char errMsg[BUFSIZ];
    va_list arglist;
    va_start(arglist, format);

    vsnprintf(errMsg, BUFSIZ, format, arglist);
    fprintf(stderr, "ERROR in [%s]: %s\n", func, errMsg);
    exit(EXIT_FAILURE);
}

/* xml functions */
xmlDocPtr getxmlDoc(const char *docname)
{
    xmlDocPtr doc;
    doc = xmlParseFile(docname);

    if (doc == NULL)
	errExit(__func__, "Unable to parse [%s]\n", docname);

    return doc;
}

xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    context = xmlXPathNewContext(doc);

    if (context == NULL)
	errExit(__func__, "xmlXPathNewContext returned NULL");
    if (xmlXPathRegisterNs(context,  BAD_CAST NSPREFIX, BAD_CAST NSURI) != 0)
	errExit(__func__,"Unable to register NS with prefix");

    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);

    if (result == NULL)
	errExit(__func__, "xmlXPathEvalExpression returned NULL");
    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
	xmlXPathFreeObject(result);
	printf("No result\n");
	return NULL;
    }
    return result;
}

void createXLzip(const char *s)
{
    char t[strlen(s) + 1];
    char *pt = t;
    const char *ps = s;
    char dirname[strlen(s) + 1];

    if (strstr(s, ".xls") == NULL)
	errExit(__func__, "[%s] not a valid excel file\n", s);

    while (*ps != '.' && *ps != '\0')
	*pt++ = *ps++;
    *pt = '\0';
    strcpy(dirname, t);

    strcat(t, ".zip");
    
    /* remove previous zip file, if it exists */
    rmrf(t);

    if (cp(t, s) != 0)
    {
	perror("cp");
	errExit(__func__, "Failed to create zip file [%s]\n", t);
    }

    /* remove previous folder, if it exists */
    rmrf(dirname);

    if (mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
	errExit(__func__, "Error making directory [%s]\n", dirname);
}

int rmrf(const char *s)
{
    int flags = FTW_CHDIR | FTW_DEPTH | FTW_MOUNT | FTW_PHYS;
    if (nftw(s, rm, 10, flags) == -1)
    {
	/* ignore no such file or directory error */
	if (errno == ENOENT || errno == ENOTDIR)
	    return 0;
	else
	{
	    perror("nftw");
	    errExit(__func__, "nftw failed\n");
	}
    }
    return 0;
}

int rm(const char *s, const struct stat *sbuf, int type, struct FTW *ftwb)
{
    remove(s);
}

int cp(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
	return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0777);
    if (fd_to < 0)
	goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
	char *out_ptr = buf;
	ssize_t nwritten;

	do {
	    nwritten = write(fd_to, out_ptr, nread);

	    if (nwritten >= 0)
	    {
		nread -= nwritten;
		out_ptr += nwritten;
	    }
	    else if (errno != EINTR)
	    {
		goto out_error;
	    }
	} while (nread > 0);
    }

    if (nread == 0)
    {
	if (close(fd_to) < 0)
	{
	    fd_to = -1;
	    goto out_error;
	}
	close(fd_from);

	/* Success! */
	return 0;
    }

out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
	close(fd_to);

    errno = saved_errno;
    return -1;
}
