main: main.o common.o lex.o type.o ast.o print_ast.o parse.o
	gcc main.o common.o lex.o type.o ast.o print_ast.o parse.o -o main

main.o: main.c lex.h common.h
	gcc -c main.c

common.o: common.c common.h
	gcc -c common.c

lex.o: lex.c lex.h common.h
	gcc -c lex.c

type.o: type.c type.h common.h
	gcc -c type.c

ast.o: ast.c ast.h lex.h common.h
	gcc -c ast.c

print_ast.o: print_ast.c ast.h
	gcc -c print_ast.c

parse.o: parse.c parse.h ast.h lex.h common.h
	gcc -c parse.c

clean:
	rm *.o
