#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <assert.h>

#include "errorexit.h"
#include "common.h"
#include "libraryheader.h"

typedef enum TypeKind {
	TYPE_DOUBLE,
	TYPE_STRING,
} TypeKind;

typedef struct Content {
	TypeKind kind;
	union {
		long long i;
		double d;
		const char *s;	
	} val;
} Content;

typedef struct Sheet {
	const char *excel_name;
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

void print_content(Content *content)
{
	if (!content) {
		printf("(Empty cell)\n");	
		return;
	}
	switch (content->kind) {
		case TYPE_DOUBLE:
			printf("%f\n", content->val.d);
			break;
		default:
			assert(TYPE_STRING == content->kind);
			printf("%s\n", content->val.s);
			break;
	}
}

const char *str_cast(const xmlChar *xml_str)
{
	const xmlChar *orig = xml_str;
	const char *str = (const char *)orig;
	while (*xml_str) {
		if (*xml_str++ > 127) {
			printf("Warning cast '%s' to char *\n", orig);
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
			printf("Warning cast '%s' to xmlChar *\n", orig);
		}
	}

	return xml_str;
}

Sheet *new_sheet(const char *file_name, const char *name, const char *sheetId)
{
	Sheet *s = jalloc(1, sizeof(*s));
	s->excel_name = file_name;
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

enum fatality {
	BENIGN,
	FATAL,
};

xmlAttrPtr find_attr(xmlAttrPtr attr, const char *name)
{
	int len = strlen(name);
	while (attr &&
			(xmlStrlen(attr->name) != len
			 || xmlStrncmp(attr->name, xml_cast(name), len)))
	{
		attr = attr->next;
	}
	return attr;
}

xmlNodePtr find_node(xmlNodePtr node, const char *name, enum fatality fat)
{
	int len = strlen(name);
	while (node &&
			(xmlStrlen(node->name) != len
			 || xmlStrncmp(node->name, xml_cast(name), len)))
	{
		node = node->next;
	}
	if (fat && !node) {
		die("FATAL: unable to find node '%s'", name);
	}
	return node;
}

#define PARSE(n, n_name) \
	assert(sheet); \
	int len = strlen(n_name); \
	for (xmlNodePtr n = node; n; n = n->next) { \
		assert(!xmlStrncmp(n->name, xml_cast(n_name), len)); \
		parse_##n(sheet, n); \
	}

char *parse_shared_strings_concat(xmlNodePtr it)
{
	xmlNodePtr node = find_node(it->children, "r", FATAL);
	char *buf = 0;
	for (xmlNodePtr it = node; it; it = it->next) {
		xmlNodePtr tmp = find_node(it->children, "t", FATAL);
		assert(tmp->children);
		assert(tmp->children->name);
		assert(!xmlStrcmp(tmp->children->name, xml_cast("text")));
		assert(tmp->children->content);
		buf_printf(buf, "%s", str_cast(tmp->children->content));
	}
	assert(buf);
	return buf;
}

char *parse_shared_strings_value(xmlNodePtr it)
{
	char *buf = 0;
	xmlNodePtr node = find_node(it->children, "t", BENIGN);
	if (!node) {
		buf = parse_shared_strings_concat(it);
	} else {
		node = find_node(node->children, "text", FATAL);
		assert(node->content);
		buf_printf(buf, "%s", str_cast(node->content));
	}
	assert(buf);
	return buf;
}

char *parse_shared_strings(unsigned long index, Sheet *sheet)
{
	xmlDocPtr sharedStrings = open_xml_from_excel(
			sheet->excel_name, "sharedStrings");
	char *buf = 0;
	unsigned long i = 0;
	for (xmlNodePtr it = sharedStrings->children->children; it;
			it = it->next)
	{
		assert(!xmlStrcmp(it->name, xml_cast("si")));
		if (i++ == index) {
			buf = parse_shared_strings_value(it);
			break;
		}
	}
	assert(buf);
	return buf;
}

Content parse_value(Sheet *sheet, xmlNodePtr node)
{
	assert(sheet);
	assert(node);
	xmlAttrPtr attr = find_attr(node->properties, "t");
	if (!attr) {
		assert(!find_node(node->children, "v", BENIGN));
		return (Content){.kind = TYPE_STRING, .val.s = ""};
	}

	xmlNodePtr test = find_node(attr->children, "text", FATAL);
	xmlNodePtr value = find_node(node->children, "v", FATAL);
	assert(value->children);
	value = find_node(value->children, "text", FATAL);
	assert(value->content);

	Content content = (Content){0};
	const char *value_str = str_cast(value->content);
	char *buf = 0;

	if (!xmlStrcmp(test->content, xml_cast("s"))) {
		content.kind = TYPE_STRING;
		char *end = 0;
		long index = strtol(value_str, &end, 0);
		assert('\0' == *end);
		assert(!(index < 0));
		buf = parse_shared_strings(index, sheet);
		assert(buf);
		content.val.s = buf;
	} else if (!xmlStrcmp(test->content, xml_cast("str"))
			|| !xmlStrcmp(test->content, xml_cast("e"))
			|| !xmlStrcmp(test->content, xml_cast("n"))) {
		/* TODO: "n" should be treated as double (so removed from
		   above */
		content.kind = TYPE_STRING;
		buf_printf(buf, "%s", value_str);
		assert(buf);
		content.val.s = buf;
	} else {
		assert(!xmlStrcmp(test->content, xml_cast("n")));
		content.kind = TYPE_DOUBLE;
		char *end = 0;
		content.val.d = strtod(value_str, &end);
		assert('\0' == *end);
	}
	return content;
}

void parse_col(Sheet *sheet, xmlNodePtr node)
{
	assert(sheet);
	assert(node);
	xmlAttrPtr attr = find_attr(node->properties, "r");
	assert(attr);
	xmlNodePtr cell_name = find_node(attr->children, "text", FATAL);
	const char *key = str_cast(cell_name->content);
	Content *content = jalloc(1, sizeof(content));
	*content = parse_value(sheet, node);
	map_put_str(sheet->cells, key, content);
}

void parse_cols(Sheet *sheet, xmlNodePtr node)
{
	PARSE(col, "c");
}

void parse_row(Sheet *sheet, xmlNodePtr node)
{
	assert(sheet);
	assert(node);
	parse_cols(sheet, node->children);	
}

void parse_rows(Sheet *sheet, xmlNodePtr node)
{
	if (!node) {
		printf("Warning: sheet '%s' is empty.\n", sheet->name);
		return;
	}
	PARSE(row, "row");
}

#undef PARSE

void parse_cells(const char *file_name, Sheet *sheet)
{
	char sheet_name[BUFSIZ];
	snprintf(sheet_name, sizeof(sheet_name), "%s%s",
			"worksheets/sheet", sheet->sheetId);
	xmlDocPtr xml_sheet = open_xml_from_excel(file_name, sheet_name);
	xmlNodePtr node = xml_sheet->children->children;
	node = find_node(node, "sheetData", FATAL);
	parse_rows(sheet, node->children);
}

Sheet *parse_sheet(const char *file_name, xmlNodePtr node)
{
	assert(node);
	const char *name = parse_attr(node->properties, "name");	
	const char *sheetId = parse_attr(node->properties, "sheetId");

	Sheet *sheet = new_sheet(file_name, name, sheetId);
	parse_cells(file_name, sheet);
	return sheet;
}

Sheet **parse_sheets(const char *file_name, const char *sheet_name)
{
	xmlDocPtr workbook = open_xml_from_excel(file_name, "workbook");
	assert(workbook);
	Sheet **sheets = 0;
	xmlNodePtr node = workbook->children->children;
	assert(node);
	node = find_node(node, "sheets", FATAL);
	const char *curr_sheet_name = 0;
	for (xmlNodePtr it = node->children; it; it = it->next) {
		curr_sheet_name = parse_attr(it->properties, "name");
		if (!sheet_name || !strcmp(sheet_name, curr_sheet_name)) {
			buf_push(sheets, parse_sheet(file_name, it));
		}
	}
	return sheets;
}

Excel *open_excel(const char *file_name, const char *sheet_name)
{
	assert(file_name);
	extract_excel(file_name);
	Excel *file = jalloc(1, sizeof(*file));
	file->name = strdup(file_name);
	file->sheets = parse_sheets(file->name, sheet_name);
	if (!file->sheets) {
		assert(sheet_name);
		die("No sheet '%s' found in file '%s'", sheet_name, file_name);
	}
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

Content *cell_content(Excel *excel, const char *sheet_name, const char *cell)
{
	if (!excel || !cell) {
		die("excel file and cell must have a value");
	}

	if (!sheet_name) {
		if (1 != buf_len(excel->sheets)) {
			die("Multiple sheets in open excel file\n"
					"Choose sheet in which the cell lies");
		} else {
			assert(excel->sheets[0]);
			return map_get_str(excel->sheets[0]->cells, cell);
		}
	} else {
		for (size_t i = 0; i < buf_len(excel->sheets); i++) {
			if (!strcmp(sheet_name, excel->sheets[i]->name)) {
				assert(excel->sheets[i]);
				return map_get_str(excel->sheets[i]->cells,
						cell);
			}
		}
	}
	die("No sheet '%s' found in file '%s'", sheet_name, excel->name);
	return 0;
}

int main(int argc, char *argv[])
{
	if (2 != argc) {
		printf("./a.out 'excel file'\n");
		return 1;
	}

	const char *file_name = argv[1];
	Excel *file = open_excel(file_name, 0);
	print_excel(file);
	const char *test_cells[] = {
		"C11",		
		"L13",		
		"AA329",		
		"D330",		
		"LV316",
		"A10000",	
	};
	for (size_t i = 0; i < sizeof(test_cells)/sizeof(test_cells[0]); i++)
	{
		Content *content = cell_content(file, "Calcul", test_cells[i]);
		print_content(content);
	}
	return 0;
}
