#include <stdlib.h>
#include <assert.h>
#include "helperfunctions.h"
#include "type.h"
#include "common.h"

Type *type_boolean = &(Type){TYPE_BOOLEAN, {{0}}};
Type *type_int = &(Type){TYPE_INT, {{0}}};
Type *type_double = &(Type){TYPE_DOUBLE, {{0}}};
Type *type_string = &(Type){TYPE_STRING, {{0}}};

Type *type_double_func; // to be inited

static double min(double *params, size_t num_params)
{
	assert(params);
	assert(num_params);
	double result = *params;
	for (size_t i = 1; i < num_params; i++) {
		result = MIN(result, params[i]);
	}
	return result;
}

static double max(double *params, size_t num_params)
{
	assert(params);
	assert(num_params);
	double result = *params;
	for (size_t i = 1; i < num_params; i++) {
		result = MAX(result, params[i]);
	}
	return result;
}

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
unsigned sym_push_func(const char *name, Type *type, double_func func);

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

static void init_builtin_func(const char *name, Type *type, double_func func)
{
	sym_push_func(name, type, func);
}

static void init_type_double_func(void)
{
	type_double_func = jalloc(1, sizeof(*type_double_func));
	*type_double_func = (Type){
		.kind = TYPE_FUNC, .func.params = 0, .func.num_params = 0,
			.func.has_varargs = 1,
			.func.varargs_type = type_double,
			.func.ret = type_double
	};
}

void init_builtin_funcs(void)
{
	init_type_double_func();
	init_builtin_func("min", type_double_func, min);
	init_builtin_func("max", type_double_func, max);
}
