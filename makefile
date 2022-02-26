all: main

main: build_msg main.o common.o lex.o type.o ast.o print_ast.o parse.o resolve.o
	gcc main.o common.o lex.o type.o ast.o print_ast.o parse.o resolve.o \
	-o main

main.o: lex.h common.h
main.o: main.c
	gcc -c main.c

common.o: common.h
common.o: common.c
	gcc -c common.c

lex.o: lex.h common.h
lex.o: lex.c
	gcc -c lex.c

type.o: type.h common.h
type.o: type.c
	gcc -c type.c

ast.o: ast.h lex.h common.h
ast.o: ast.c
	gcc -c ast.c

print_ast.o: ast.h
print_ast.o: print_ast.c
	gcc -c print_ast.c

parse.o: parse.h ast.h lex.h common.h
parse.o: parse.c
	gcc -c parse.c

resolve.o: resolve.h
resolve.o: resolve.c
	gcc -c resolve.c

.PHONY: clean
clean:
	rm *.o
	rm main

build_msg:
	@printf "#\n# Building main \n#\n"
