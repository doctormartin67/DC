main: main.o common.o lex.o
	gcc main.o common.o lex.o

main.o: main.c lex.h common.h
	gcc -c main.c

common.o: common.c common.h
	gcc -c common.c

lex.o: lex.c lex.h
	gcc -c lex.c
