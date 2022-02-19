#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <assert.h>

#include "errorexit.h"
#include "common.h"
#include "libraryheader.h"

typedef struct Sheet {
	const char *name;
	const char *sheetId;
	Map *cells;
} Sheet;

typedef struct Excel {
	const char *name;
	Sheet **sheets;
} Excel;

void remove_dir(const char *dir_name)
{
	assert(dir_name);
	char cmd[BUFSIZ];
	snprintf(cmd, sizeof(cmd), "%s%s%c", "rm -rf '", dir_name, '\'');
	if (system(cmd) != 0)
		die("system command [%s] failed, are you on windows?", cmd);
}

void unzip_zip(const char *zip_name, const char *dir_name)
{
	assert(zip_name && dir_name);
	char cmd[BUFSIZ];

	remove_dir(dir_name);

	if (system(cmd) != 0)
		die("system command [%s] failed, are you on windows?", cmd);

	if (mkdir(dir_name, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
		die("Error making directory [%s]", dir_name);

	snprintf(cmd, sizeof(cmd), "%s%s%s%s%c", 
			"unzip -q '", zip_name, "' -d '", dir_name, '\'');

	if (system(cmd) != 0)
		die("system command [%s] failed, are you on windows?", cmd);
}

void create_zip(const char *zip_name, const char *file_name)
{
	assert(zip_name && file_name);
	char cmd[BUFSIZ];

	remove_dir(zip_name);

	if (system(cmd) != 0)
		die("system command [%s] failed, are you on windows?", cmd);

	snprintf(cmd, sizeof(cmd), 
			"%s%s%c%s%s%c", "cp '", file_name, '\'', " '",
			zip_name, '\'');

	if (system(cmd) != 0)
		die("system command [%s] failed, are you on windows?", cmd);

}

void extract_excel(const char *file_name)
{
	assert(file_name);
	size_t len = strlen(file_name) + 1;
	char dir_name[len];
	char zip_name[PATH_MAX + 1];
	char *pd = dir_name;
	const char *prevpxls = file_name, *pxls = 0, *pf = file_name;

	while (0 != (prevpxls = strstr(prevpxls, ".xls"))) {
		pxls = prevpxls++;
	}

	if (0 == pxls) die("[%s] not a valid excel file", file_name);

	while (pf < pxls) *pd++ = *pf++;
	*pd = '\0';

	snprintf(zip_name, sizeof(zip_name), "%s.zip", dir_name);

	create_zip(zip_name, file_name);
	unzip_zip(zip_name, dir_name);
}

xmlDocPtr open_xml_from_excel(const char *file_name, const char *xml_name)
{
	assert(file_name && xml_name);
	char tmp[BUFSIZ];
	const char *start = file_name;
	const char *end = file_name;
	xmlDocPtr doc = 0;

	end += strlen(end) - 1;

	switch (*end) {
		case 'x':
		case 'm':
			if ('s' == *(end - 1)
					&& 'l' == *(end - 2)
					&& 'x' == *(end - 3)) {
				end -= 4;
			} else {
				die("'%s' is not an excel file", file_name);
			}
			break;

		case 's':
			if ('l' == *(end - 1) && 'x' == *(end - 2)) {
				end -= 3;
			} else {
				die("'%s' is not an excel file", file_name);
			}
			break;
		default:
			die("'%s' is not an excel file", file_name);
			break;
	}
	snprintf(tmp, sizeof(tmp), "%.*s%s%s%s", (int)(end - start), start,
			"/xl/", xml_name, ".xml");

	if (0 == (doc = xmlParseFile(tmp))) {
		die("Unable to parse xml file '%s'", file_name);
	}
	return doc;
}

static int indent;

void print_indent(void)
{
	for (int i = indent; i > 0; i--) {
		printf("   ");
	}
}

void print_xmlNode(xmlNodePtr node);

void print_xmlAttr(xmlAttrPtr attr)
{
	if (attr) {
		print_indent();
		printf("attr:\n");
		indent++;
		print_indent();
		printf("name: %s\n", attr->name);
		indent--;
	} else {
		return;
	}

	assert(attr);

	indent++;
	print_xmlNode(attr->children);
	indent--;
}

void print_xmlAttrs(xmlAttrPtr attr)
{
	for (xmlAttrPtr it = attr; it; it = it->next) {
		print_xmlAttr(it);
	}
}

void print_xmlChildNode(xmlNodePtr child)
{
	for (xmlNodePtr node = child; node; node = node->next) {
		print_xmlNode(node);
	}
}

void print_xmlNode(xmlNodePtr node)
{
	if (node) {
		print_indent();
		printf("node:\n");
		indent++;
		print_indent();
		printf("name: %s\n", node->name);
		print_indent();
		printf("content: %s\n", node->content);
		print_xmlAttrs(node->properties);
		indent--;
	} else {
		return;
	}

	assert(node);

	indent++;
	print_xmlChildNode(node->children);
	indent--;
}

void print_xmlDoc(xmlDocPtr doc)
{
	printf("doc:\n");
	indent++;
	print_indent();
	printf("name: %s\n", doc->name);
	for (xmlNodePtr node = doc->children; node; node = node->next) {
		print_xmlNode(node);
	}
	for (xmlNodePtr node = doc->next; node; node = node->next) {
		print_xmlNode(node);
	}
	indent = 0;
}

const char *str_cast(const xmlChar *xml_str)
{
	const xmlChar *orig = xml_str;
	const char *str = (const char *)orig;
	while (*xml_str) {
		if (*xml_str++ > 127) {
			die("Unable to cast '%s' to char *", orig);
		}
	}

	return str;
}

const xmlChar *xml_cast(const char *str)
{
	const char *orig = str;
	const xmlChar *xml_str = (const xmlChar *)orig;
	while (*str) {
		if (*str++ < 0) {
			die("Unable to cast '%s' to xmlChar *", orig);
		}
	}

	return xml_str;
}

Sheet *new_sheet(const char *name, const char *sheetId)
{
	Sheet *s = jalloc(1, sizeof(*s));
	s->name = name;
	s->sheetId = sheetId;
	s->cells = jalloc(1, sizeof(*s->cells));
	return s;
}

const char *parse_attr(xmlAttrPtr attr, const char *attr_name)
{
	assert(attr);
	const char *content = 0;
	int len = strlen(attr_name);
	while (attr && 	
			(xmlStrlen(attr->name) != len
			 || xmlStrncmp(attr->name, xml_cast(attr_name), len)))
	{
		attr = attr->next;
	}
	if (!attr) {
		die("FATAL: No attribute called '%s'", attr_name);
	} else if (!attr->children) {
		die("FATAL: No child node for attribute '%s'", attr_name);
	} else if (!attr->children->content) {
		die("FATAL: No content in child node of attribute '%s'",
				attr_name);
	}

	content = strdup(str_cast(attr->children->content));
	return content;
}

xmlNodePtr find_node(xmlNodePtr node, const char *name)
{
	int len = strlen(name);
	while (node &&
			(xmlStrlen(node->name) != len
			 || xmlStrncmp(node->name, xml_cast(name), len)))
	{
		node = node->next;
	}
	return node;
}

void parse_cells(const char *file_name, Sheet *sheet)
{
	char sheet_name[BUFSIZ];
	snprintf(sheet_name, sizeof(sheet_name), "%s%s",
			"worksheets/sheet", sheet->sheetId);
	xmlDocPtr xml_sheet = open_xml_from_excel(file_name, sheet_name);
	xmlNodePtr node = xml_sheet->children->children;
	node = find_node(node, "sheetData");
	if (!node) {
		die("FATAL: unable to find sheetData in sheet '%s'",
				sheet->name);
	}
	print_xmlNode(node);
}

Sheet *parse_sheet(const char *file_name, xmlNodePtr node)
{
	assert(node);
	const char *name = parse_attr(node->properties, "name");	
	const char *sheetId = parse_attr(node->properties, "sheetId");

	Sheet *sheet = new_sheet(name, sheetId);
	parse_cells(file_name, sheet);
	return sheet;
}

Sheet **parse_sheets(const char *file_name)
{
	xmlDocPtr workbook = open_xml_from_excel(file_name, "workbook");
	assert(workbook);
	Sheet **sheets = 0;
	xmlNodePtr node = workbook->children->children;
	assert(node);
	node = find_node(node, "sheets");
	if (!node) {
		die("FATAL: No sheets in workbook");
	}

	for (xmlNodePtr it = node->children; it; it = it->next) {
		buf_push(sheets, parse_sheet(file_name, it));
	}
	return sheets;
}

Excel *open_excel(const char *file_name)
{
	assert(file_name);
	extract_excel(file_name);
	Excel *file = jalloc(1, sizeof(*file));
	file->name = strdup(file_name);
	file->sheets = parse_sheets(file->name);
	return file;
}

void print_sheet(Sheet *s)
{
	printf("sheet name: %s\n", s->name);
	printf("sheet Id: %s\n", s->sheetId);
}

void print_excel(Excel *e)
{
	printf("file name: %s\n", e->name);	
	printf("sheets:\n");
	for (size_t i = 0; i < buf_len(e->sheets); i++) {
		print_sheet(e->sheets[i]);
	}
}

int main(int argc, char *argv[])
{
	if (2 != argc) {
		printf("./a.out 'excel file'\n");
		return 1;
	}

	const char *file_name = argv[1];
	Excel *file = open_excel(file_name);

	return 0;
}
