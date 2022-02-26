main: main.o common.o lex.o type.o ast.o
	gcc main.o common.o lex.o type.o ast.o -o main

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
