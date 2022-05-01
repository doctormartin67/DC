#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <assert.h>
#include <locale.h> /* used for strtod strange '.' truncation */

#include "excel.h"
#include "errorexit.h"
#include "common.h"
#include "helperfunctions.h"

static Arena content_arena;

static void remove_dir(const char *dir_name)
{
	assert(dir_name);
	char cmd[BUFSIZ];
	snprintf(cmd, sizeof(cmd), "%s%s%c", "rm -rf '", dir_name, '\'');
	if (system(cmd) != 0)
		die("system command [%s] failed, are you on windows?", cmd);
}

static void unzip_zip(const char *zip_name, const char *dir_name)
{
	assert(zip_name && dir_name);
	char cmd[BUFSIZ];

	remove_dir(dir_name);

	if (mkdir(dir_name, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
		die("Error making directory [%s]", dir_name);

	snprintf(cmd, sizeof(cmd), "%s%s%s%s%c", 
			"unzip -q '", zip_name, "' -d '", dir_name, '\'');

	if (system(cmd) != 0)
		die("system command [%s] failed, are you on windows?", cmd);
}

static void create_zip(const char *zip_name, const char *file_name)
{
	assert(zip_name && file_name);
	char cmd[BUFSIZ];

	remove_dir(zip_name);

	snprintf(cmd, sizeof(cmd), 
			"%s%s%c%s%s%c", "cp '", file_name, '\'', " '",
			zip_name, '\'');

	if (system(cmd) != 0)
		die("system command [%s] failed, are you on windows?", cmd);

}

static void extract_excel(const char *file_name)
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

static xmlDocPtr open_xml_from_excel(const char *file_name,
		const char *xml_name)
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
		die("Unable to parse xml file '%s'", tmp);
	}
	return doc;
}

static int indent;

static void print_indent(void)
{
	for (int i = indent; i > 0; i--) {
		printf("   ");
	}
}

static void print_xmlNode(xmlNodePtr node);

static void print_xmlAttr(xmlAttrPtr attr)
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

static void print_xmlAttrs(xmlAttrPtr attr)
{
	for (xmlAttrPtr it = attr; it; it = it->next) {
		print_xmlAttr(it);
	}
}

static void print_xmlChildNode(xmlNodePtr child)
{
	for (xmlNodePtr node = child; node; node = node->next) {
		print_xmlNode(node);
	}
}

static void print_xmlNode(xmlNodePtr node)
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

#if 0
static void print_xmlDoc(xmlDocPtr doc)
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
#endif

static const char *str_cast(const xmlChar *xml_str)
{
	const xmlChar *orig = xml_str;
	const char *str = (const char *)orig;
	return str;
}

static const xmlChar *xml_cast(const char *str)
{
	const char *orig = str;
	const xmlChar *xml_str = (const xmlChar *)orig;
	return xml_str;
}

static Sheet *new_sheet(const char *file_name, char *name, unsigned sheetId)
{
	Sheet *s = jalloc(1, sizeof(*s));
	s->excel_name = file_name;
	s->name = name;
	s->sheetId = sheetId;
	s->cells = jalloc(1, sizeof(*s->cells));
	return s;
}

static char *parse_attr(xmlAttrPtr attr, const char *attr_name)
{
	assert(attr);
	char *content = 0;
	const xmlChar *xml_name = xml_cast(attr_name);
	int len = strlen(attr_name);
	while (attr && 	
			(xmlStrlen(attr->name) != len
			 || xmlStrncmp(attr->name, xml_name, len)))
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

static xmlAttrPtr find_attr(xmlAttrPtr attr, const char *name)
{
	int len = strlen(name);
	const xmlChar *xml_name = xml_cast(name);
	while (attr &&
			(xmlStrlen(attr->name) != len
			 || xmlStrncmp(attr->name, xml_name, len)))
	{
		attr = attr->next;
	}
	return attr;
}

static xmlNodePtr find_node(xmlNodePtr node, const char *name,
		enum fatality fat)
{
	int len = strlen(name);
	const xmlChar *xml_name = xml_cast(name);
	while (node &&
			(xmlStrlen(node->name) != len
			 || xmlStrncmp(node->name, xml_name, len)))
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
	const xmlChar *xml_name = xml_cast(n_name); \
	for (xmlNodePtr n = node; n; n = n->next) { \
		assert(!xmlStrncmp(n->name, xml_name, len)); \
		parse_##n(sheet, n); \
	}

static char *parse_shared_strings_concat(xmlNodePtr it)
{
	xmlNodePtr node = find_node(it->children, "r", FATAL);
	char *buf = 0;
	const xmlChar *text = xml_cast("text");
	for (xmlNodePtr it = node; it; it = it->next) {
		xmlNodePtr tmp = find_node(it->children, "t", FATAL);
		assert(tmp->children);
		assert(tmp->children->name);
		assert(!xmlStrcmp(tmp->children->name, text));
		assert(tmp->children->content);
		buf_printf(buf, "%s", str_cast(tmp->children->content));
	}
	assert(buf);
	return buf;
}

static char *parse_shared_strings_value(xmlNodePtr it)
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

/*
 * This is used so that parse_shared_strings doesnt have to open and close
 * it each time the function is called. it is created in parse_sheets and then
 * destroyed afterwards
 */
static xmlDocPtr sharedStrings;
static Content parse_shared_strings(ContentKind kind, const char *value)
{
	assert(sharedStrings);
	Content content = (Content){0};
	content.kind = kind;
	char *buf = 0;
	char *end = 0;
	long i = 0;
	long index = strtol(value, &end, 0);
	assert('\0' == *end);
	assert(!(index < 0));
	const xmlChar *si = xml_cast("si");
	for (xmlNodePtr it = sharedStrings->children->children; it;
			it = it->next)
	{
		assert(!xmlStrcmp(it->name, si));
		if (i++ == index) {
			buf = parse_shared_strings_value(it);
			break;
		}
	}
	assert(buf);
	content.val.s = arena_str_dup(&content_arena, buf);
	buf_free(buf);
	return content;
}

static Content parse_string(ContentKind kind, const char *value)
{
	char *buf = 0;
	Content content = (Content){0};
	content.kind = kind;
	buf_printf(buf, "%s", value);
	assert(buf);
	content.val.s = arena_str_dup(&content_arena, buf);
	buf_free(buf);
	return content;
}

static unsigned is_excel_float(const char *str)
{
	assert(str);
	while (*str) {
		if ('.' == *str || 'e' == *str || 'E' == *str) {
			return 1;
		}
		str++;
	}
	return 0;
}

static Content parse_number(const char *value)
{
	char *end = 0;
	Content content = (Content){0};
	if (is_excel_float(value)) {
		content.kind = CONTENT_DOUBLE;
		char *saved_locale = setlocale(LC_NUMERIC, "C");
		content.val.d = strtod(value, &end);
		assert('\0' == *end);
		setlocale(LC_NUMERIC, saved_locale);
	} else {
		content.kind = CONTENT_INT;
		content.val.i = strtol(value, &end, 10);
		assert('\0' == *end);
	}
	return content;
}

static Content parse_boolean(const char *value)
{
	char *end = 0;
	long long i = strtol(value, &end, 10);
	assert(true == i || false == i);
	Content content = (Content){0};
	content.kind = CONTENT_BOOLEAN;
	content.val.b = i;
	assert('\0' == *end);
	return content;
}

static Content parse_content(Sheet *sheet, const xmlChar *type,
		xmlNodePtr value)
{
	assert(sheet);
	assert(value);
	Content content = (Content){0};
	const char *value_str = str_cast(value->content);

	if (!xmlStrcmp(type, xml_cast("s"))) {
		content = parse_shared_strings(CONTENT_STRING, value_str);
		assert(CONTENT_STRING == content.kind);
	} else if (!xmlStrcmp(type, xml_cast("str"))) {
		content = parse_string(CONTENT_STRING, value_str);
		assert(CONTENT_STRING == content.kind);
	} else if (!xmlStrcmp(type, xml_cast("e"))) {
		content = parse_string(CONTENT_ERROR, "");
		assert(CONTENT_ERROR == content.kind);
	} else if (!xmlStrcmp(type, xml_cast("b"))) {
		content = parse_boolean(value_str);
		assert(CONTENT_BOOLEAN == content.kind);
	} else {
		assert(!xmlStrcmp(type, xml_cast("n")));
		content = parse_number(value_str);
		assert(CONTENT_INT == content.kind
				|| CONTENT_DOUBLE == content.kind);
	}
	assert(CONTENT_NONE != content.kind);
	return content;
}

static void print_sheet(Sheet *s);
static Content parse_value(Sheet *sheet, xmlNodePtr node)
{
	assert(sheet);
	assert(node);
	xmlAttrPtr attr = find_attr(node->properties, "t");
	xmlNodePtr test = 0;
	xmlNodePtr value = 0;
	const xmlChar *type = 0;
	if (!attr) {
		/* 
		 * value with no type will be treated as number,
		 * this can happen with f.e. dates. otherwise it is treated
		 * as empty string
		 */
		if ((value = find_node(node->children, "v", BENIGN))) {
			type = xml_cast("n");
		} else {
			return (Content){0};
		}
	} else {
		test = find_node(attr->children, "text", FATAL);
		value = find_node(node->children, "v", FATAL);
		type = test->content;
	}

	assert(type);
	assert(value->children);
	value = find_node(value->children, "text", FATAL);
	assert(value->content);

	return parse_content(sheet, type, value);
}

static void parse_col(Sheet *sheet, xmlNodePtr node)
{
	assert(sheet);
	assert(node);
	xmlAttrPtr attr = find_attr(node->properties, "r");
	assert(attr);
	xmlNodePtr cell_name = find_node(attr->children, "text", FATAL);
	const char *key = str_cast(cell_name->content);
	Content *content = arena_alloc(&content_arena, sizeof(*content));
	*content = parse_value(sheet, node);
	assert(content);
	map_put_str(sheet->cells, key, content);
}

static void parse_cols(Sheet *sheet, xmlNodePtr node)
{
	PARSE(col, "c");
}

static void parse_row(Sheet *sheet, xmlNodePtr node)
{
	assert(sheet);
	assert(node);
	parse_cols(sheet, node->children);	
}

static void parse_rows(Sheet *sheet, xmlNodePtr node)
{
	if (!node) {
		printf("Warning: sheet '%s' is empty.\n", sheet->name);
		return;
	}
	PARSE(row, "row");
}

#undef PARSE

static void parse_cells(const char *file_name, Sheet *sheet)
{
	char sheet_name[BUFSIZ];
	snprintf(sheet_name, sizeof(sheet_name), "%s%u",
			"worksheets/sheet", sheet->sheetId);
	xmlDocPtr xml_sheet = open_xml_from_excel(file_name, sheet_name);
	xmlNodePtr node = xml_sheet->children->children;
	node = find_node(node, "sheetData", FATAL);
	parse_rows(sheet, node->children);
	xmlFreeDoc(xml_sheet);
}

static Sheet *parse_sheet(const char *file_name, xmlNodePtr node,
		unsigned sheetId)
{
	assert(node);
	char *name = parse_attr(node->properties, "name");	

	Sheet *sheet = new_sheet(file_name, name, sheetId);
	parse_cells(file_name, sheet);
	return sheet;
}

static Sheet **parse_sheets(const char *file_name, const char *sheet_name)
{
	sharedStrings = open_xml_from_excel(file_name, "sharedStrings");
	xmlDocPtr workbook = open_xml_from_excel(file_name, "workbook");
	assert(workbook);
	Sheet **sheets = 0;
	xmlNodePtr node = workbook->children->children;
	assert(node);
	node = find_node(node, "sheets", FATAL);
	char *curr_sheet_name = 0;
	unsigned sheetId = 0;
	for (xmlNodePtr it = node->children; it; it = it->next) {
		sheetId++;
		curr_sheet_name = parse_attr(it->properties, "name");
		if (!sheet_name || !strcmp(sheet_name, curr_sheet_name)) {
			buf_push(sheets, parse_sheet(file_name, it, sheetId));
		}
		free(curr_sheet_name);
	}
	xmlFreeDoc(sharedStrings);
	xmlFreeDoc(workbook);
	return sheets;
}

Excel *open_excel(const char *file_name, const char *sheet_name)
{
	assert(file_name);
	extract_excel(file_name);
	Excel *file = jalloc(1, sizeof(*file));
	file->name = file_name;
	file->sheets = parse_sheets(file->name, sheet_name);
	if (!file->sheets) {
		assert(sheet_name);
		die("No sheet '%s' found in file '%s'", sheet_name, file_name);
	}
	return file;
}

static void free_sheets(Sheet **sheets)
{
	for (size_t i = 0; i < buf_len(sheets); i++) {
		map_free(sheets[i]->cells);
		free(sheets[i]->cells);
		free(sheets[i]->name);
		free(sheets[i]);
	}
	buf_free(sheets);
}

void close_excel(Excel *excel)
{
	free_sheets(excel->sheets);
	arena_free(&content_arena);
	free(excel);
	xmlCleanupParser();
}

static void print_sheet(Sheet *s)
{
	printf("sheet name: %s\n", s->name);
	printf("sheet Id: %u\n", s->sheetId);
}

void print_excel(Excel *e)
{
	printf("file name: %s\n", e->name);	
	printf("sheets:\n");
	for (size_t i = 0; i < buf_len(e->sheets); i++) {
		print_sheet(e->sheets[i]);
	}
}

static void cast_to_string(Content *content, ContentKind kind)
{
	assert(CONTENT_STRING != content->kind);
	char *buf = 0;
	switch (kind) {
		case CONTENT_BOOLEAN:
			buf_printf(buf, "%d", content->val.b);
			break;
		case CONTENT_INT:
			buf_printf(buf, "%d", content->val.i);
			break;
		case CONTENT_DOUBLE:
			buf_printf(buf, "%f", content->val.d);
			break;
		default:
			assert(0);
			break;
	}
	assert(buf);
	content->val.s = arena_str_dup(&content_arena, buf);
	content->kind = CONTENT_STRING;
	buf_free(buf);
}

#define CAST(t) \
	switch (kind) { \
		case CONTENT_BOOLEAN: \
				      content->val.b = content->val.t; \
		break; \
		case CONTENT_INT: \
				  content->val.i = content->val.t; \
		break; \
		case CONTENT_DOUBLE: \
				     content->val.d = content->val.t; \
		break; \
		case CONTENT_STRING: \
				     cast_to_string(content, content->kind); \
		assert(CONTENT_STRING == content->kind); \
		break; \
		case CONTENT_ERROR: \
				    assert(0); \
		break; \
		default: \
			 assert(0); \
		break; \
	}

static Content *default_content(ContentKind kind)
{
	Content *content = arena_alloc(&content_arena, sizeof(*content));
	switch (kind) {
		case CONTENT_BOOLEAN:
			content->val.b = false;
			break;
		case CONTENT_INT:
			content->val.i = 0;
			break;
		case CONTENT_DOUBLE:
			content->val.d = 0.0;
			break;
		case CONTENT_STRING:
		case CONTENT_ERROR:
			content->val.s = "";
			break;
		case CONTENT_NONE:
			assert(0);
			break;
		default:
			break;
	}
	content->kind = kind;
	return content;
}

static void cast_content(Content *content, ContentKind kind)
{
	if (kind == content->kind) {
		return;
	}

	switch (content->kind) {
		case CONTENT_BOOLEAN:
			CAST(b);
			break;
		case CONTENT_INT:
			CAST(i);
			break;
		case CONTENT_DOUBLE:
			CAST(d);
			break;
		case CONTENT_STRING:
		case CONTENT_ERROR:
		case CONTENT_NONE:
			/*
			 * we will not cast from string to number, if we've
			 * reached this stage it means theres some format
			 * issue or something and then the excel file should
			 * be altered instead. in this case we just cast to
			 * default values
			 */
			*content = *default_content(kind);
			assert(kind == content->kind);
			break;
		default:
			assert(0);
			break;
	}
	content->kind = kind;
}

#undef CAST

static Content *record_content(const Database *db, size_t num_record,
		const char *title)
{
	return map_get_str(db->records[num_record]->data, title);
}

static unsigned is_title(const Database *db, const char *title)
{
	for (size_t i = 0; i < buf_len(db->titles); i++) {
		if (!strcmp(title, db->titles[i])) {
			return 1;	
		}
	}
	return 0;
}

bool record_boolean(const Database *db, size_t num_record, const char *title)
{
	if (!is_title(db, title)) {
		die("Could not find title '%s' in the database", title);
	}
	Content *content = record_content(db, num_record, title);
	if (!content) {
		content = default_content(CONTENT_BOOLEAN);
	}
	cast_content(content, CONTENT_BOOLEAN);
	assert(CONTENT_BOOLEAN == content->kind);
	return content->val.b;
}

int record_int(const Database *db, size_t num_record, const char *title)
{
	if (!is_title(db, title)) {
		die("Could not find title '%s' in the database", title);
	}
	Content *content = record_content(db, num_record, title);
	if (!content) {
		content = default_content(CONTENT_BOOLEAN);
	}
	cast_content(content, CONTENT_INT);
	assert(CONTENT_INT == content->kind);
	return content->val.i;
}

double record_double(const Database *db, size_t num_record, const char *title)
{
	if (!is_title(db, title)) {
		die("Could not find title '%s' in the database", title);
	}
	Content *content = record_content(db, num_record, title);
	if (!content) {
		content = default_content(CONTENT_BOOLEAN);
	}
	cast_content(content, CONTENT_DOUBLE);
	assert(CONTENT_DOUBLE == content->kind);
	return content->val.d;
}

const char *record_string(const Database *db, size_t num_record,
		const char *title)
{
	if (!is_title(db, title)) {
		die("Could not find title '%s' in the database", title);
	}
	Content *content = record_content(db, num_record, title);
	if (!content) {
		content = default_content(CONTENT_BOOLEAN);
	}
	cast_content(content, CONTENT_STRING);
	assert(CONTENT_STRING == content->kind);
	return content->val.s;
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
			assert(excel->sheets[0]->cells);
			return map_get_str(excel->sheets[0]->cells, cell);
		}
	} else {
		for (size_t i = 0; i < buf_len(excel->sheets); i++) {
			if (!strcmp(sheet_name, excel->sheets[i]->name)) {
				assert(excel->sheets[i]);
				assert(excel->sheets[i]->cells);
				return map_get_str(excel->sheets[i]->cells,
						cell);
			}
		}
	}
	die("No sheet '%s' found in file '%s'", sheet_name, excel->name);
	return 0;
}

Val cell_val(Excel *excel, const char *sheet_name, const char *cell)
{
	Content *content = cell_content(excel, sheet_name, cell);
	if (content) {
		return content->val;
	} else {
		return (Val){.s = "0"};
	}
}

static const char *parse_title(Content *content)
{
	const char *title = 0;
	char *buf = 0;
	switch (content->kind) {
		case CONTENT_BOOLEAN:
			if (content->val.b) {
				buf_printf(buf, "%s", "true");
			} else {
				buf_printf(buf, "%s", "false");
			}
			title = arena_str_dup(&content_arena, buf);
			buf_free(buf);
			break;
		case CONTENT_INT:
			buf_printf(buf, "%d", content->val.i);
			title = arena_str_dup(&content_arena, buf);
			buf_free(buf);
			break;
		case CONTENT_DOUBLE:
			buf_printf(buf, "%f", content->val.d);
			title = arena_str_dup(&content_arena, buf);
			buf_free(buf);
			break;
		case CONTENT_STRING:
		case CONTENT_ERROR:
			title = content->val.s;
			break;
		default:
			assert(0);
			break;
	}
	assert(title);
	return title;
}

static char *nextcol(char next[static 1]);
static const char **parse_titles(Excel *excel, const char *sheet_name,
		const char *cell)
{
	char curr_cell[CELL_LENGTH];
	const char **titles = 0;
	Content *content = cell_content(excel, sheet_name, cell);
	if (!content) {
		print_excel(excel);
		die("No database found in:\n"
				"file: '%s'\n"
				"sheet: '%s'\n"
				"cell '%s'", excel->name, sheet_name, cell);
	}

	snprintf(curr_cell, sizeof(curr_cell), "%s", cell);
	for (Content *cnt = content; cnt; cnt = cell_content(excel, sheet_name,
				nextcol(curr_cell))) {
		if (CONTENT_NONE == cnt->kind) {
			break;
		}
		buf_push(titles, parse_title(cnt));		
	}
	return titles;
}

static Database *new_database(Excel *excel, const char **titles,
		Record **records)
{
	Database *db = arena_alloc(&content_arena, sizeof(*db));
	db->excel = excel;
	db->titles = titles;
	db->num_titles = buf_len(titles);
	db->records = records;
	db->num_records = buf_len(records);
	return db;
}

static size_t double_titles(const char **titles)
{
	for (size_t i = 0; i < buf_len(titles); i++) {
		for (size_t j = i + 1; j < buf_len(titles); j++) {
			if (!strcmp(titles[i], titles[j])) {
				return j;
			}
		}
	}
	return 0;
}

static Record *new_record(Map *data, const char **titles)
{
	Record *record = arena_alloc(&content_arena, sizeof(*record));
	record->data = data;	
	record->titles = titles;
	record->num_titles = buf_len(titles);
	return record;
}

static Record *parse_record(const char **titles, Excel *excel,
		const char *sheet_name, const char *cell)
{
	char curr_cell[CELL_LENGTH];
	size_t cnt_empty = 0;
	snprintf(curr_cell, sizeof(curr_cell), "%s", cell);
	Map *data = jalloc(1, sizeof(*data));
	Content *content = 0;
	Record *record = 0;
	for (size_t i = 0; i < buf_len(titles); i++) {
		content = cell_content(excel, sheet_name, curr_cell);
		if (!content) {
			cnt_empty++;
		}
		map_put_str(data, titles[i], content);
		nextcol(curr_cell);
	}
	if (cnt_empty == buf_len(titles)) {
		map_free(data);
		free(data);
	} else {
		record = new_record(data, titles);
	}
	return record;
}

static Record **parse_records(const char **titles, Excel *excel,
		const char *sheet_name, const char *cell)
{
	char curr_cell[CELL_LENGTH];
	snprintf(curr_cell, sizeof(curr_cell), "%s", cell);
	Record **records = 0;
	Record *it = parse_record(titles, excel, sheet_name,
			nextrow(curr_cell));
	if (!it) {
		die("Empty database at:\n"
				"file: '%s'\n"
				"sheet: '%s'\n"
				"cell: '%s'\n", excel->name, sheet_name, cell);
	}
	while (1) {
		buf_push(records, it);
		it = parse_record(titles, excel, sheet_name,
				nextrow(curr_cell));
		if (!it) {
			break;
		}
	}
	assert(records);
	return records;
}

static Database *parse_database(Excel *excel, const char *sheet_name,
		const char *cell)
{
	const char **titles = parse_titles(excel, sheet_name, cell);
	size_t is_double_title = double_titles(titles);
	size_t cnt_double = 0;
	while (is_double_title) {
		char *buf = 0;
		buf_printf(buf, "%s%d", titles[is_double_title], ++cnt_double);
		titles[is_double_title] = arena_str_dup(&content_arena, buf);
		buf_free(buf);
		is_double_title = double_titles(titles);
	}
	Record **records = parse_records(titles, excel, sheet_name, cell);
	Database *db = new_database(excel, titles, records);
	return db;
}

/*
 * a Database will start at the upper left and the titles of the Database
 * will be to the right of this cell (cell included) and the records of the
 * Database will be downwards
 */
Database *open_database(const char *file_name, const char *sheet_name,
		const char *cell)
{
	Excel *excel = open_excel(file_name, sheet_name);
	assert(excel);
	Database *db = parse_database(excel, sheet_name, cell);
	assert(db);
	return db;
}

static void close_records(Record **records)
{
	for (size_t i = 0; i < buf_len(records); i++) {
		map_free(records[i]->data);
		free(records[i]->data);
	}
	buf_free(records);
}

void close_database(Database *db)
{
	buf_free(db->titles);
	close_records(db->records);
	close_excel(db->excel);
}

#if 0
static void print_titles(Database *db)
{
	for (size_t i = 0; i < db->num_titles; i++) {
		printf("%s ", db->titles[i]);
	}
	printf("\n");
}

static void print_content(Content *content)
{
	switch (content->kind) {
		case CONTENT_BOOLEAN:
			if (content->val.b) {
				printf("true"); 
			} else {
				printf("false");
			}
			break;
		case CONTENT_INT:
			printf("%d", content->val.i);
			break;
		case CONTENT_DOUBLE:
			printf("%f", content->val.d);
			break;
		case CONTENT_STRING:
		case CONTENT_ERROR:
			printf("%s", content->val.s);
			break;
		case CONTENT_NONE:
			assert(0);
			break;
		default:
			assert(0);
			break;
	}
}

static void print_record(Record *record) {
	for (size_t i = 0; i < record->num_titles; i++) {
		print_content(map_get_str(record->data, record->titles[i]));
		if (!(i == record->num_titles - 1)) {
			printf(" ");
		}
	}
}

static void print_records(Database *db)
{
	for (size_t i = 0; i < db->num_records; i++) {
		print_record(db->records[i]);
		if (!(i == db->num_records - 1)) {
			printf(" ");
			printf("\n");
		}
	}
	printf("\n");
}

static void print_database(Database *db)
{
	print_titles(db);
	print_records(db);
}
#endif

/*
 * helper function for nextcol. it shifts all the char's one place to the
 * right. It is needed in the case where the column is 'Z' or 'ZZ', meaning
 * the next column would be 'AA' or 'AAA' respectively. As an example,
 * Z11 becomes AA11
 */
static void strshift(char s[static 1])
{
	char *t = s;

	while (*s) s++;
	*(s + 1) = '\0';

	while (s > t) {
		*s = *(s - 1);
		s--;
	}
}

/*
 * sets 'next' to the next cell to the right, f.e. B11 -> C11
 */
static char *nextcol(char next[static 1])
{
	char *s = next;
	if ('Z' == *s && (isdigit(*(s + 1)) || 'Z' == *(s + 1))) {
		strshift(s);
		*s = 'A';
		*(s + 1) = 'A';
		if (!isdigit(*(s + 2)))
			*(s + 2) = 'A';

		return next;
	}

	while (!isdigit(*s) && '\0' != *s) {
		if (*s <= 'z' && *s >= 'a') die("invalid cell [%s]", next);
		s++;
	}
	s--;
	while ('Z' == *s) *s-- = 'A';
	(*s)++;
	return next;
}

char *nextrow(char next[static 1])
{
	char *start = next;
	assert(!isdigit(*start));
	while (!isdigit(*start)) {
		start++;
	}
	char *end = next + strlen(next);
	char *s = end - 1;
	while ('9' == *s) {
		*s-- = '0';
	}
	if (s == start - 1) {
		*++s = '1';
		*end = '0';
		*(end + 1) = '\0';
	} else {
		(*s)++;
	}
	return next;
}
