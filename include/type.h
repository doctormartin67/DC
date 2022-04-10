#ifndef TYPE_H_
#define TYPE_H_

#include <stddef.h>

typedef enum TypeKind {
	TYPE_NONE,
	TYPE_INCOMPLETE,
	TYPE_COMPLETING,
	TYPE_BOOLEAN,
	TYPE_INT,
	TYPE_ULLONG,
	TYPE_DOUBLE,
	TYPE_STRING,
	TYPE_FUNC,
	NUM_TYPE_KINDS,
} TypeKind;

typedef struct Type Type;
typedef struct Sym Sym;

struct Type {
	TypeKind kind;
	union {
		struct {
			Type **params;
			size_t num_params;
			unsigned has_varargs;
			Type *varargs_type;
			Type *ret;
		} func;
	};
};
typedef double double_func(double *params, size_t num_params);

void init_builtin_types(void);
void init_builtin_funcs(void);
void clear_builtin_types(void);
const char *type_name(TypeKind);
unsigned is_integer_type(Type *type);
unsigned is_floating_type(Type *type);
unsigned is_string_type(Type *type);
unsigned is_arithmetic_type(Type *type);
unsigned is_scalar_type(Type *type);
unsigned is_concatable(Type *type);

#endif
