#ifndef INTERPRET_H_
#define INTERPRET_H_

#include "common.h"
#include "type.h"
#include "excel.h"

Val interpret(const char *vba_code, TypeKind return_type, unsigned loop,
		const Database *db, size_t num_record);
void add_builtin_int(const char *name, int i);
void add_builtin_boolean(const char *name, bool b);
void add_builtin_double(const char *name, int d);
void add_builtin_string(const char *name, const char *s);

#endif
