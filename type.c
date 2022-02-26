#include <stdlib.h>
#include "type.h"
#include "common.h"

Type *type_boolean = &(Type){TYPE_BOOLEAN};
Type *type_int = &(Type){TYPE_INT};
Type *type_double = &(Type){TYPE_DOUBLE};
Type *type_string = &(Type){TYPE_STRING};

static const char *const type_names[NUM_TYPE_KINDS] = {
	[TYPE_BOOLEAN] = "boolean",
	[TYPE_INT] = "integer",
	[TYPE_DOUBLE] = "double",
	[TYPE_STRING] = "string",
};

const char *type_name(TypeKind kind)
{
	switch (kind) {
		case TYPE_BOOLEAN:
		case TYPE_INT:
		case TYPE_DOUBLE:
		case TYPE_STRING:
			return type_names[kind];
		default:
			fatal("Unknown type");
			return 0;
	}
}

unsigned is_boolean_type(Type *type)
{
	return TYPE_BOOLEAN == type->kind;
}

unsigned is_integer_type(Type *type)
{
	return TYPE_INT <= type->kind && type->kind <= TYPE_INT;
}

unsigned is_floating_type(Type *type)
{
	return TYPE_DOUBLE <= type->kind && type->kind <= TYPE_DOUBLE;
}

unsigned is_string_type(Type *type)
{
	return TYPE_STRING == type->kind;
}

unsigned is_arithmetic_type(Type *type)
{
	return TYPE_BOOLEAN <= type->kind && type->kind <= TYPE_DOUBLE;
}

unsigned is_scalar_type(Type *type)
{
	return TYPE_INT <= type->kind && type->kind <= TYPE_DOUBLE;
}

unsigned is_concatable(Type *type)
{
	return TYPE_INT <= type->kind && type->kind <= TYPE_STRING;
}

unsigned sym_push_type(const char *name, Type *type);

static void init_builtin_type(Type *type)
{
	sym_push_type(type_names[type->kind], type);
}

void init_builtin_types(void)
{
	init_builtin_type(type_boolean);
	init_builtin_type(type_int);
	init_builtin_type(type_double);
	init_builtin_type(type_string);
}
