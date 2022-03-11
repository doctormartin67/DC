#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <assert.h>

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
		die("Unable to parse xml file '%s'", file_name);
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
	while (*xml_str) {
		if (*xml_str++ > 127) {
			printf("Warning cast '%s' to char *\n", orig);
		}
	}

	return str;
}

static const xmlChar *xml_cast(const char *str)
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

static Sheet *new_sheet(const char *file_name, char *name, char *sheetId)
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

static xmlAttrPtr find_attr(xmlAttrPtr attr, const char *name)
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

static xmlNodePtr find_node(xmlNodePtr node, const char *name,
		enum fatality fat)
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

static char *parse_shared_strings_concat(xmlNodePtr it)
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

static Content parse_shared_strings(TypeKind kind, const char *value,
		Sheet *sheet)
{
	xmlDocPtr sharedStrings = open_xml_from_excel(
			sheet->excel_name, "sharedStrings");
	Content content = (Content){0};
	content.kind = kind;
	char *buf = 0;
	char *end = 0;
	long i = 0;
	long index = strtol(value, &end, 0);
	assert('\0' == *end);
	assert(!(index < 0));
	for (xmlNodePtr it = sharedStrings->children->children; it;
			it = it->next)
	{
		assert(!xmlStrcmp(it->name, xml_cast("si")));
		if (i++ == index) {
			buf = parse_shared_strings_value(it);
			break;
		}
	}
	xmlFreeDoc(sharedStrings);
	assert(buf);
	content.val.s = arena_str_dup(&content_arena, buf);
	buf_free(buf);
	return content;
}

static Content parse_string(TypeKind kind, const char *value)
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
		content.kind = TYPE_DOUBLE;
		content.val.d = strtod(value, &end);
		assert('\0' == *end);
	} else {
		content.kind = TYPE_INT;
		content.val.i = strtol(value, &end, 10);
		assert('\0' == *end);
	}
	return content;
}

static Content parse_content(Sheet *sheet, const xmlChar *type,
		xmlNodePtr value)
{
	Content content = (Content){0};
	const char *value_str = str_cast(value->content);

	if (!xmlStrcmp(type, xml_cast("s"))) {
		content = parse_shared_strings(TYPE_STRING, value_str, sheet);
		assert(TYPE_STRING == content.kind);
	} else if (!xmlStrcmp(type, xml_cast("str"))) {
		content = parse_string(TYPE_STRING, value_str);
		assert(TYPE_STRING == content.kind);
	} else if (!xmlStrcmp(type, xml_cast("e"))) {
		content = parse_string(TYPE_ERROR, "");
		assert(TYPE_ERROR == content.kind);
	} else {
		assert(!xmlStrcmp(type, xml_cast("n")));
		content = parse_number(value_str);
		assert(TYPE_INT == content.kind
				|| TYPE_DOUBLE == content.kind);
	}
	return content;
}

static Content parse_value(Sheet *sheet, xmlNodePtr node)
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

	return parse_content(sheet, test->content, value);
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
	snprintf(sheet_name, sizeof(sheet_name), "%s%s",
			"worksheets/sheet", sheet->sheetId);
	xmlDocPtr xml_sheet = open_xml_from_excel(file_name, sheet_name);
	xmlNodePtr node = xml_sheet->children->children;
	node = find_node(node, "sheetData", FATAL);
	parse_rows(sheet, node->children);
	xmlFreeDoc(xml_sheet);
}

static Sheet *parse_sheet(const char *file_name, xmlNodePtr node)
{
	assert(node);
	char *name = parse_attr(node->properties, "name");	
	char *sheetId = parse_attr(node->properties, "sheetId");

	Sheet *sheet = new_sheet(file_name, name, sheetId);
	parse_cells(file_name, sheet);
	return sheet;
}

static Sheet **parse_sheets(const char *file_name, const char *sheet_name)
{
	xmlDocPtr workbook = open_xml_from_excel(file_name, "workbook");
	assert(workbook);
	Sheet **sheets = 0;
	xmlNodePtr node = workbook->children->children;
	assert(node);
	node = find_node(node, "sheets", FATAL);
	char *curr_sheet_name = 0;
	for (xmlNodePtr it = node->children; it; it = it->next) {
		curr_sheet_name = parse_attr(it->properties, "name");
		if (!sheet_name || !strcmp(sheet_name, curr_sheet_name)) {
			buf_push(sheets, parse_sheet(file_name, it));
		}
		free(curr_sheet_name);
	}
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
		free(sheets[i]->sheetId);
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
		case TYPE_INT:
			buf_printf(buf, "%d", content->val.i);
			title = arena_str_dup(&content_arena, buf);
			buf_free(buf);
			break;
		case TYPE_DOUBLE:
			buf_printf(buf, "%f", content->val.d);
			title = arena_str_dup(&content_arena, buf);
			buf_free(buf);
			break;
		case TYPE_STRING:
		case TYPE_ERROR:
			title = content->val.s;
			break;
		default:
			assert(0);
			break;
	}
	assert(title);
	return title;
}

static const char **parse_titles(Excel *excel, const char *sheet_name,
		const char *cell)
{
	char curr_cell[CELL_LENGTH];
	const char **titles = 0;
	Content *content = cell_content(excel, sheet_name, cell);
	if (!content) {
		die("No database found in:\n"
				"file: '%s'"
				"sheet: '%s'"
				"cell '%s'", excel->name, sheet_name, cell);
	}

	snprintf(curr_cell, sizeof(curr_cell), "%s", cell);
	for (Content *cnt = content; cnt; cnt = cell_content(excel, sheet_name,
				nextcol(curr_cell))) {
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
	if (is_double_title) {
		die("unable to create database because of multiple titles of "
				"the same name '%s':\n"
				"file: '%s'\n"
				"sheet: '%s'\n"
				"cell: '%s'", titles[is_double_title],
				excel->name, sheet_name, cell);
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
		case TYPE_INT:
			printf("%d", content->val.i);
			break;
		case TYPE_DOUBLE:
			printf("%f", content->val.d);
			break;
		case TYPE_STRING:
		case TYPE_ERROR:
			printf("%s", content->val.s);
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

void print_database(Database *db)
{
	print_titles(db);
	print_records(db);
}
