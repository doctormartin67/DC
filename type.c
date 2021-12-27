typedef enum TypeKind {
	TYPE_NONE,
	TYPE_INCOMPLETE,
	TYPE_COMPLETING,
	TYPE_BOOLEAN,
	TYPE_INT,
	TYPE_DOUBLE,
	NUM_TYPE_KINDS,
} TypeKind;

typedef struct Type Type;
typedef struct Sym Sym;

struct Type {
	TypeKind kind;
	TODO(size_t size);
	TODO(size_t align);
	TODO(size_t padding);
	Sym *sym;
	Type *base;
	unsigned typeid;
	unsigned nonmodifiable;
	union {
		struct {
			size_t num_elems;
			unsigned incomplete_elems;
		};
	};
};

typedef struct TypeMetrics {
	size_t size;
	TODO(size_t align);
	unsigned sign;
	unsigned long long max;
} TypeMetrics;

TypeMetrics *type_metrics;

Type *type_boolean = &(Type){TYPE_BOOLEAN};
Type *type_int = &(Type){TYPE_INT};
Type *type_double = &(Type){TYPE_DOUBLE};

unsigned next_typeid = 1;

Type *type_usize;
Type *type_ssize;

Type *type_any;

Map typeid_map;

Type *get_type_from_typeid(int typeid) {
	if (typeid == 0) {
		return 0;
	}
	return map_get(&typeid_map, (void *)(uintptr_t)typeid);
}

void register_typeid(Type *type) {
	map_put(&typeid_map, (void *)(uintptr_t)type->typeid, type);
}

Type *type_alloc(TypeKind kind) {
	Type *type = calloc(1, sizeof(*type));
	type->kind = kind;
	type->typeid = next_typeid++;
	register_typeid(type);
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

unsigned is_signed_type(Type *type) {
	switch (type->kind) {
		case TYPE_INT:
			return 1;
		default:
			return 0;
	}
}

const char *const type_names[NUM_TYPE_KINDS] = {
	[TYPE_BOOLEAN] = "boolean",
	[TYPE_INT] = "integer",
	[TYPE_DOUBLE] = "double",
};

unsigned type_ranks[NUM_TYPE_KINDS] = {
	[TYPE_BOOLEAN] = 1,
	[TYPE_INT] = 2,
};

unsigned type_rank(Type *type) {
	unsigned rank = type_ranks[type->kind];
	assert(0 != rank);
	return rank;
}

Type *unsigned_type(Type *type) {
	switch (type->kind) {
		case TYPE_BOOLEAN:
			return type_boolean;
		default:
			assert(0);
			return 0;
	}
}

/*
 * TODO These two functions could be used to add qualifiers later
 */
Type *unqualify_type(Type *type) {
	return type;
}

Type *qualify_type(Type *type, Type *qual) {
	type = unqualify_type(type);
	return type;
}

typedef struct TypeLink {
	Type *type;
	struct TypeLink *next;
} TypeLink;

Type *type_incomplete(Sym *sym) {
	Type *type = type_alloc(TYPE_INCOMPLETE);
	type->sym = sym;
	return type;
}

void init_builtin_type(Type *type) {
	type->typeid = next_typeid++;
	register_typeid(type);
	TODO(type->size = type_metrics[type->kind].size);
	TODO(type->align = type_metrics[type->kind].align);
}

void init_builtin_types(void) {
	init_builtin_type(type_boolean);
	init_builtin_type(type_int);
	init_builtin_type(type_double);
}
