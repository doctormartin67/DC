#ifndef TYPE_H_
#define TYPE_H_

typedef enum TypeKind {
	TYPE_NONE,
	TYPE_INCOMPLETE,
	TYPE_COMPLETING,
	TYPE_BOOLEAN,
	TYPE_INT,
	TYPE_ULLONG,
	TYPE_DOUBLE,
	TYPE_STRING,
	NUM_TYPE_KINDS,
} TypeKind;

typedef struct Type Type;
typedef struct Sym Sym;

struct Type {
	TypeKind kind;
};

void init_builtin_types(void);
const char *type_name(TypeKind);
unsigned is_integer_type(Type *type);
unsigned is_floating_type(Type *type);
unsigned is_string_type(Type *type);
unsigned is_arithmetic_type(Type *type);
unsigned is_scalar_type(Type *type);
unsigned is_concatable(Type *type);

#endif
