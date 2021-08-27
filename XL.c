#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "libraryheader.h"
#include "XL.h"
#include "errorexit.h"

/* 
 * uses xmlDocPtr to represent xml files that will be used to retrieve values
 * from cells in the given excel file 's'. If the excel file could not be
 * created the function returns NULL
 */
XLfile *createXL(const char *s)
{
	XLfile *xl = jalloc(1, sizeof(XLfile));
	*xl = (XLfile){0};

	char temp[BUFSIZ];
	char *pt = 0;

	snprintf(temp, sizeof(temp), "%s", s);
	snprintf(xl->fname, sizeof(xl->fname), "%s", s);
	snprintf(xl->dirname, sizeof(xl->dirname), "%s", s);
	if (0 == (pt = strstr(xl->dirname, ".xls")))
		errExit("[%s] [%s] not an excel file\n", __func__, s);
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

	xl->sheetname = jalloc(MAXSHEETS, sizeof(char *));
	setsheetnames(xl);
	xl->sheets = jalloc(MAXSHEETS, sizeof(xmlDocPtr));

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
char *cell(const XLfile *xl, unsigned sheet, const char *s)
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
		errExit("[%s] no value found in cell [%s]\n", __func__, s);

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
		errExit("[%s] no element <v> for cell [%s]\n", __func__, s);

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
char *findss(const XLfile *xl, int index)
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
		errExit("[%s] The nodes have no childs, " 
				"but the childs hold the string values\n", 
				__func__);

	if (0 == xmlStrcmp(childnode->name, (const xmlChar *)"t")) {
		s = (char *)xmlNodeGetContent(childnode);
	} else if (0 == xmlStrcmp(childnode->name, (const xmlChar *)"r")) {
		s = getconcatss(childnode);
	} else
		errExit("[%s] Unknown element [%s]\n", 
				__func__, childnode->name);

	if (0 == s) errExit("[%s] string to return is NULL\n", __func__);

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

		if (0 == gcn)
			errExit("[%s] no \"t\" element " 
					"in sharedStrings.xml\n", __func__);

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
void setsheetnames(XLfile *xl)
{
	char **xls = xl->sheetname;
	xmlNodePtr p = xmlDocGetRootElement(xl->workbook);

	if (0 == p) errExit("[%s] Empty document\n", __func__);

	if (0 != xmlStrcmp(p->name, (const xmlChar *) "workbook"))
		errExit("[%s] root node != workbook\n", __func__);

	for (p = p->children; 0 != p; p = p->next)
		if ((0 == xmlStrcmp(p->name, (const xmlChar *)"sheets")))
			break;

	if (0 == p) errExit("[%s] No \"sheets\" node\n", __func__);

	for (xmlNodePtr ps = p->children; 0 != ps; ps = ps->next)
		*xls++ = (char *)xmlGetProp(ps, (const xmlChar *)"name");
	*xls = 0;
}

/*
 * This function uses XPATH functionality of the libxml library to set all the
 * nodes of sharedStrings and the sheets array.
 */
void setnodes(XLfile *xl)
{
	xl->nodesetss = getnodeset(xl->sharedStrings, (xmlChar *)XPATHSS);
	if (0 == (xl->nodesetss->nodesetval))
		errExit("[%s] there are no nodes in sharedStrings.xml\n", 
				__func__);

	xl->nodesets = jalloc(xl->sheetcnt, sizeof(xmlXPathObjectPtr));
	for (unsigned i = 0; i < xl->sheetcnt; i++) {
		xl->nodesets[i] 
			= getnodeset(xl->sheets[i], (xmlChar *)XPATHDATA);

		if (0 == (xl->nodesets[i]->nodesetval))
			errExit("[%s] there are no nodes in sheet [%s]\n", 
					__func__,  xl->sheetname[i]);
	}
}

/* 
 * This function will return the row of a given excel cell,
 * for example 11 in the case of "B11"
 */
unsigned getrow(const char *cell)
{
	while (!isdigit(*cell)) cell++;
	return atoi(cell);
}

/* 
 * This function will set s to the column of a given excel cell,
 * for example "B11" will set s to "B"
 */
void setcol(char s[], const char *cell)
{
	while (!isdigit(*cell)) *s++ = *cell++;
	*s = '\0';
}

/*
 * sets 'next' to the next cell to the right, f.e. B11 -> C11
 */
void nextcol(char *next)
{
	char *s = next;
	if ('Z' == *s && (isdigit(*(s + 1)) || 'Z' == *(s + 1))) {
		strshift(s);
		*s = 'A';
		*(s + 1) = 'A';
		if (!isdigit(*(s + 2)))
			*(s + 2) = 'A';

		return;
	}

	while (!isdigit(*s) && '\0' != *s) {
		if (*s <= 'z' && *s >= 'a')
			errExit("[%s] invalid cell [%s]\n", __func__, next);

		s++;
	}
	s--;
	while ('Z' == *s) *s-- = 'A';
	(*s)++;
}

/*
 * helper function for nextcol. it shifts all the char's one place to the
 * right. It is needed in the case where the column is 'Z' or 'ZZ', meaning
 * the next column would be 'AA' or 'AAA' respectively. As an example,
 * Z11 becomes AA11
 */
void strshift(char *s)
{
	char *t = s;

	while (*s) s++;
	*(s + 1) = '\0';

	while (s > t) {
		*s = *(s - 1);
		s--;
	}
}

/* returns xmlDocPtr or NULL if docname could not be found */
xmlDocPtr getxmlDoc(const char *docname)
{
	xmlDocPtr doc;
	if(0 == (doc = xmlParseFile(docname)))
		fprintf(stderr, "[%s] Unable to parse file [%s]\n", __func__, docname);

	return doc;
}

xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	context = xmlXPathNewContext(doc);

	if (0 == context)
		errExit("[%s] xmlXPathNewContext returned NULL\n", __func__);
	if (xmlXPathRegisterNs(context,  BAD_CAST NSPREFIX, BAD_CAST NSURI) != 0)
		errExit("[%s] Unable to register NS with prefix\n", __func__);

	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);

	if (0 == result)
		errExit("[%s] xmlXPathEvalExpression returned NULL\n", __func__);
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
		xmlXPathFreeObject(result);
		printf("No result\n");
		return 0;
	}
	return result;
}

/* --- free memory function --- */

void freeXL(XLfile *xl)
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

void createXLzip(const char *s)
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

	if (0 == pxls) 
		errExit("[%s] [%s] not a valid excel file\n", __func__, s);

	while (ps < pxls) *pt++ = *ps++;
	*pt = '\0';
	strcpy(dirname, t);

	strcat(t, ".zip");

	/* remove previous zip file, if it exists */
	snprintf(cmd, sizeof(cmd), "%s%s%c", "rm -rf '", t, '\'');

	/* THIS IS NOT PORTABLE !!! */
	if (system(cmd) != 0)
		errExit("[%s] system command [%s] failed, " 
				"are you on windows?\n", __func__, cmd);

	snprintf(cmd, sizeof(cmd), 
			"%s%s%c%s%s%c", "cp '", s, '\'', " '", t, '\'');

	/* THIS IS NOT PORTABLE !!! */
	if (system(cmd) != 0)
		errExit("[%s] system command [%s] failed, " 
				"are you on windows?\n", __func__, cmd);

	/* remove previous folder, if it exists */
	snprintf(cmd, sizeof(cmd), "%s%s%c", "rm -rf '", dirname, '\'');

	/* THIS IS NOT PORTABLE !!! */
	if (system(cmd) != 0)
		errExit("[%s] system command [%s] failed, " 
				"are you on windows?\n", __func__, cmd);

	if (mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
		errExit("[%s] Error making directory [%s]\n", 
				__func__, dirname);

	snprintf(cmd, sizeof(cmd), "%s%s%s%s%c", 
			"unzip -q '", t, "' -d '", dirname, '\'');

	/* THIS IS NOT PORTABLE !!! */
	if (system(cmd) != 0)
		errExit("[%s] system command [%s] failed, " 
				"are you on windows?\n", __func__, cmd);
}
