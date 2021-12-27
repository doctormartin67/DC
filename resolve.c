typedef enum SymKind {
	SYM_NONE,
	SYM_DIM,
	SYM_FUNC,
	SYM_TYPE,
} SymKind;

typedef struct Sym {
	const char *name;
	SymKind kind;
	Decl *decl;
	struct {
		Type *type;
		Val val;
	};
} Sym;

enum {
	MAX_SYMS = 1024
};

Sym syms[MAX_SYMS];
Sym *syms_end = syms;

Sym *sym_new(SymKind kind, const char *name, Decl *decl)
{
	Sym *sym = calloc(1, sizeof(*sym));
	sym->kind = kind;
	sym->name = name;
	sym->decl = decl;
	return sym;
}

Sym *sym_decl(Decl *decl)
{
	SymKind kind = SYM_NONE;
	switch (decl->kind) {
		case DECL_DIM:
			kind = SYM_DIM;
			break;
		default:
			assert(0);
			break;
	}
	Sym *sym = sym_new(kind, decl->name, decl);
	return sym;
}

Sym *sym_get(const char *name)
{
	for (Sym *it = syms_end; it != syms; it--) {
		Sym *sym = it - 1;
		if (sym->name == name) {
			return sym;
		}
	}
	return 0;
}

unsigned sym_push_dim(const char *name, Type *type)
{
	if (sym_get(name)) {
		return 0;
	}
	if (syms_end == syms + MAX_SYMS) {
		fatal("Too many symbols");
	}
	*syms_end++ = (Sym){
		.name = name,
			.kind = SYM_DIM,
			.type = type,
	};
	return 1;
}

void put_type_name(char **buf, Type *type)
{
	const char *type_name = type_names[type->kind];
	if (type_name) {
		buf_printf(*buf, "%s", type_name);
	} else {
		assert(0);
	}
}

char *get_type_name(Type *type)
{
	char *buf = 0;
	put_type_name(&buf, type);
	return buf;
}

typedef struct Operand {
	Type *type;
	unsigned is_lvalue;
	unsigned is_const;
	Val val;
} Operand;

Operand operand_null;

Operand operand_rvalue(Type *type)
{
	return (Operand){
		.type = unqualify_type(type),
	};
}

Operand operand_lvalue(Type *type)
{
	return (Operand){
		.type = type,
			.is_lvalue = 1,
	};
}
