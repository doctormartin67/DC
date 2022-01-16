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
	size_t size;
	Sym *sym;
};

Type *type_boolean = &(Type){TYPE_BOOLEAN};
Type *type_int = &(Type){TYPE_INT};
Type *type_double = &(Type){TYPE_DOUBLE};
Type *type_string = &(Type){TYPE_STRING};

Type *type_alloc(TypeKind kind) {
	Type *type = calloc(1, sizeof(*type));
	type->kind = kind;
	return type;
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

const char *const type_names[NUM_TYPE_KINDS] = {
	[TYPE_BOOLEAN] = "boolean",
	[TYPE_INT] = "integer",
	[TYPE_DOUBLE] = "double",
	[TYPE_STRING] = "string",
};

unsigned sym_push_type(const char *name, Type *type);

void init_builtin_type(Type *type)
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
