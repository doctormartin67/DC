#include <string.h>
#include <sys/stat.h>
#include "libraryheader.h"
#include "XL.h"

/*
 * inline functions
 */
unsigned getrow(const char *cell);
void setcol(char s[], const char *cell);
void strshift(char *s);
void nextcol(char *next);

/* 
 * uses xmlDocPtr to represent xml files that will be used to retrieve values
 * from cells in the given excel file 's'. If the excel file could not be
 * created the function returns NULL
 */
XLfile *createXL(const char s[static restrict 1])
{
	XLfile *xl = jalloc(1, sizeof(*xl));
	*xl = (XLfile){0};

	char temp[BUFSIZ];
	char *pt = 0;

	snprintf(temp, sizeof(temp), "%s", s);
	snprintf(xl->fname, sizeof(xl->fname), "%s", s);
	snprintf(xl->dirname, sizeof(xl->dirname), "%s", s);
	if (0 == (pt = strstr(xl->dirname, ".xls")))
		errExit("[%s] not an excel file", s);
	*pt = '\0';

	snprintf(temp, sizeof(temp), "%s%s", xl->dirname, 
			"/xl/workbook.xml");
	if (0 == (xl->workbook = getxmlDoc(temp))) {
		freeXL(xl);
		return 0;
	}

	snprintf(temp, sizeof(temp), "%s%s", xl->dirname, 
			"/xl/sharedStrings.xml");
	if (0 == (xl->sharedStrings = getxmlDoc(temp))) {
		freeXL(xl);
		return 0;
	}

	xl->sheetname = jalloc(MAXSHEETS, sizeof(*xl->sheetname));
	setsheetnames(xl);
	xl->sheets = jalloc(MAXSHEETS, sizeof(*xl->sheets));

	char **t = xl->sheetname;
	int i;
	for (i = 0; 0 != *t; i++, t++) {
		snprintf(temp, sizeof(temp), "%s%s%d%s", xl->dirname, 
				"/xl/worksheets/sheet", i + 1, ".xml");
		if (0 == (xl->sheets[i] = getxmlDoc(temp))) {
			freeXL(xl);
			return 0;
		}
	}
	xl->sheetcnt = i;
	setnodes(xl);

	return xl;
}

/*
 * This function uses the libxml documentation to retrieve the value of a
 * cell in a given sheet and returns this value in string form. 's' is the
 * cell to look in, f.e. "B11". If nothing was found in the cell then the
 * function returns NULL
 */
char *cell(const XLfile xl[static restrict 1], unsigned sheet,
		const char s[static restrict 1])
{
	xmlXPathObjectPtr nodeset = 0;
	xmlNodeSetPtr nodes = 0;
	xmlNodePtr node = 0;
	xmlNodePtr childnode = 0;
	xmlChar *scell = 0, *t = 0;
	char *v = 0, *row = 0;
	unsigned r = getrow(s);

	nodeset = xl->nodesets[sheet];
	nodes = nodeset->nodesetval;

	for (node = *nodes->nodeTab; 0 != node; node = node->next) {
		row = (char *)xmlGetProp(node, (const xmlChar *)"r");
		if (atoi(row) == (int)r) {
			free(row);
			break;
		} else
			free(row);
		;
	}
	if (node == 0) return 0;

	if (0 == (node = node->children))
		errExit("no value found in cell [%s]", s);

	while (node) {
		/* find cell in found row */
		scell = xmlGetProp(node, (const xmlChar *)"r");
		if (xmlStrcmp(scell, (xmlChar *)s) == 0) {
			xmlFree(scell);
			break;
		} else
			xmlFree(scell);

		node = node->next;
	}

	if (0 == node || 0 == node->children) return 0;

	childnode = node->children;
	while (childnode) {
		if (0 == xmlStrcmp(childnode->name, (const xmlChar *)"v")) {
			v = (char *)xmlNodeGetContent(childnode);
			break;
		}
		childnode = childnode->next;
	}
	if (0 == v)
		errExit("no element <v> for cell [%s]", s);

	t = xmlGetProp(node, (const xmlChar *)"t");
	if (0 == xmlStrcmp(t, (const xmlChar *)"s")) {
		int temp = atoi(v);
		xmlFree(v);
		xmlFree(t);
		return findss(xl, temp);
	} else {
		xmlFree(t);
		return v;
	}
	return 0;
}

/* the excel zip has an xml file with all the string literals called 
 * sharedStrings. In the sheet xml files they are listed as a number and so we
 * need to retrieve the strings given this number (index)
 * sharedStrings.xml either has the string located in the "t" attribute or
 * it lies within an "r" attribute which holds multiple "t" attributes that
 * need to be concatenated (don't ask me why, 
 * I figured this out all by myself)
 */
char *findss(const XLfile xl[static restrict 1], int index)
{
	char *s = 0;
	xmlXPathObjectPtr nodeset = 0;
	xmlNodeSetPtr nodes = 0;
	xmlNodePtr node = 0;
	xmlNodePtr childnode = 0;

	nodeset = xl->nodesetss;
	nodes = nodeset->nodesetval;
	node = nodes->nodeTab[index];

	if (0 == (childnode = node->children))
		errExit("The nodes have no childs, " 
				"but the childs hold the string values");

	if (0 == xmlStrcmp(childnode->name, (const xmlChar *)"t")) {
		s = (char *)xmlNodeGetContent(childnode);
	} else if (0 == xmlStrcmp(childnode->name, (const xmlChar *)"r")) {
		s = getconcatss(childnode);
	} else
		errExit("Unknown element [%s]", childnode->name);

	if (0 == s) errExit("string to return is NULL");

	return s;
}

/*
 * helper function for findss
 */
char *getconcatss(xmlNodePtr cn)
{
	char temp[BUFSIZ] = "";
	char *s = 0, *t = temp;
	xmlNodePtr gcn = 0; /*grandchild node */
	while (cn) {
		gcn = cn->children;
		while (gcn && 0 != xmlStrcmp(gcn->name, (const xmlChar *)"t"))
			gcn = gcn->next;

		if (0 == gcn) errExit("no \"t\" element in sharedStrings.xml");

		s = (char *)xmlNodeGetContent(gcn);
		snprintf(temp, sizeof(temp), "%s%s", t, s);
		xmlFree(s);
		cn = cn->next;
	}
	return strdup(temp);
}

/*
 * XLfile struct has an element sheetname which holds all the sheet names of 
 * the excel file. this function uses the workbook.xml file to set them
 */
void setsheetnames(XLfile xl[static restrict 1])
{
	char **xls = xl->sheetname;
	xmlNodePtr p = xmlDocGetRootElement(xl->workbook);

	if (0 == p) errExit("Empty document");

	if (0 != xmlStrcmp(p->name, (const xmlChar *) "workbook"))
		errExit("root node != workbook");

	for (p = p->children; 0 != p; p = p->next)
		if ((0 == xmlStrcmp(p->name, (const xmlChar *)"sheets")))
			break;

	if (0 == p) errExit("No \"sheets\" node");

	for (xmlNodePtr ps = p->children; 0 != ps; ps = ps->next)
		*xls++ = (char *)xmlGetProp(ps, (const xmlChar *)"name");
	*xls = 0;
}

/*
 * This function uses XPATH functionality of the libxml library to set all the
 * nodes of sharedStrings and the sheets array.
 */
void setnodes(XLfile xl[static restrict 1])
{
	xl->nodesetss = getnodeset(xl->sharedStrings, (xmlChar *)XPATHSS);
	if (0 == (xl->nodesetss->nodesetval))
		errExit("there are no nodes in sharedStrings.xml");

	xl->nodesets = jalloc(xl->sheetcnt, sizeof(*xl->nodesets));
	for (unsigned i = 0; i < xl->sheetcnt; i++) {
		xl->nodesets[i] 
			= getnodeset(xl->sheets[i], (xmlChar *)XPATHDATA);

		if (0 == (xl->nodesets[i]->nodesetval))
			errExit("there are no nodes in sheet [%s]",
					xl->sheetname[i]);
	}
}

/* returns xmlDocPtr or NULL if docname could not be found */
xmlDocPtr getxmlDoc(const char docname[static restrict 1])
{
	xmlDocPtr doc;
	if(0 == (doc = xmlParseFile(docname)))
		fprintf(stderr, "[%s] Unable to parse file [%s]\n",
				__func__, docname);

	return doc;
}

xmlXPathObjectPtr getnodeset(const xmlDocPtr restrict doc,
		const xmlChar xpath[static restrict 1])
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	context = xmlXPathNewContext(doc);

	if (0 == context) errExit("xmlXPathNewContext returned NULL");

	if (xmlXPathRegisterNs(context,  BAD_CAST NSPREFIX, BAD_CAST NSURI))
		errExit("Unable to register NS with prefix");

	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);

	if (0 == result) errExit("xmlXPathEvalExpression returned NULL");

	if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
		xmlXPathFreeObject(result);
		printf("No result\n");
		return 0;
	}
	return result;
}

/* --- free memory function --- */

void freeXL(XLfile *restrict xl)
{
	if (0 == xl) return;

	xmlFreeDoc(xl->workbook);
	xmlFreeDoc(xl->sharedStrings);
	for (unsigned i = 0; i < xl->sheetcnt; i++) {
		xmlFreeDoc(xl->sheets[i]);
		xmlXPathFreeObject(xl->nodesets[i]);
	}
	free(xl->sheets);
	free(xl->nodesets);
	xmlXPathFreeObject(xl->nodesetss);

	free(xl->sheetname);
	free(xl);
}

/* --- NON PORTABLE FUNCTION --- */

void createXLzip(const char s[static 1])
{
	char t[strlen(s) + 1];
	char *pt = t;
	const char *prevpxls = s, *pxls = 0, *ps = s;
	char dirname[strlen(s) + 1];
	char cmd[BUFSIZ];

	/* 
	 * find final occurrence of ".xls", usually it should only occur once, 
	 * but you never know
	 */
	while (0 != (prevpxls = strstr(prevpxls, ".xls"))) pxls = prevpxls++;

	if (0 == pxls) errExit("[%s] not a valid excel file", s);

	while (ps < pxls) *pt++ = *ps++;
	*pt = '\0';
	strcpy(dirname, t);

	strcat(t, ".zip");

	/* remove previous zip file, if it exists */
	snprintf(cmd, sizeof(cmd), "%s%s%c", "rm -rf '", t, '\'');

	/* THIS IS NOT PORTABLE !!! */
	if (system(cmd) != 0)
		errExit("system command [%s] failed, are you on windows?",
				cmd);

	snprintf(cmd, sizeof(cmd), 
			"%s%s%c%s%s%c", "cp '", s, '\'', " '", t, '\'');

	/* THIS IS NOT PORTABLE !!! */
	if (system(cmd) != 0)
		errExit("system command [%s] failed, are you on windows?",
				cmd);

	/* remove previous folder, if it exists */
	snprintf(cmd, sizeof(cmd), "%s%s%c", "rm -rf '", dirname, '\'');

	/* THIS IS NOT PORTABLE !!! */
	if (system(cmd) != 0)
		errExit("system command [%s] failed, are you on windows?",
				cmd);

	if (mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
		errExit("Error making directory [%s]", dirname);

	snprintf(cmd, sizeof(cmd), "%s%s%s%s%c", 
			"unzip -q '", t, "' -d '", dirname, '\'');

	/* THIS IS NOT PORTABLE !!! */
	if (system(cmd) != 0)
		errExit("system command [%s] failed, are you on windows?",
				cmd);
}
