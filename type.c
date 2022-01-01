typedef enum TypeKind {
	TYPE_NONE,
	TYPE_INCOMPLETE,
	TYPE_COMPLETING,
	TYPE_BOOLEAN,
	TYPE_INT,
	TYPE_ULLONG,
	TYPE_DOUBLE,
	NUM_TYPE_KINDS,
} TypeKind;

typedef struct Type Type;
typedef struct Sym Sym;

struct Type {
	TypeKind kind;
	size_t size;
	Sym *sym;
};

typedef struct TypeMetrics {
	size_t size;
	unsigned sign;
	unsigned long long max;
} TypeMetrics;

#define DEFAULT_TYPE_METRICS \
	[TYPE_BOOLEAN] = {.size = 1}, \
[TYPE_INT] = {.size = 4, .max = 0x7fffffff, .sign = 1}, \
[TYPE_DOUBLE] = {.size = 8}

TypeMetrics type_metrics[NUM_TYPE_KINDS] = {
	DEFAULT_TYPE_METRICS,
};

Type *type_boolean = &(Type){TYPE_BOOLEAN};
Type *type_int = &(Type){TYPE_INT};
Type *type_double = &(Type){TYPE_DOUBLE};

Type *type_alloc(TypeKind kind) {
	Type *type = calloc(1, sizeof(*type));
	type->kind = kind;
	return type;
}

unsigned is_integer_type(Type *type) {
	return TYPE_BOOLEAN <= type->kind && type->kind <= TYPE_INT;
}

unsigned is_floating_type(Type *type) {
	return TYPE_DOUBLE <= type->kind && type->kind <= TYPE_DOUBLE;
}

unsigned is_arithmetic_type(Type *type) {
	return TYPE_BOOLEAN <= type->kind && type->kind <= TYPE_DOUBLE;
}

unsigned is_scalar_type(Type *type) {
	return TYPE_BOOLEAN <= type->kind && type->kind <= TYPE_DOUBLE;
}

const char *const type_names[NUM_TYPE_KINDS] = {
	[TYPE_BOOLEAN] = "boolean",
	[TYPE_INT] = "integer",
	[TYPE_DOUBLE] = "double",
};

unsigned sym_push_type(const char *name, Type *type);
void init_builtin_type(Type *type) {
	sym_push_type(type_names[type->kind], type);
}

void init_builtin_types(void) {
	init_builtin_type(type_boolean);
	init_builtin_type(type_int);
	init_builtin_type(type_double);
}
