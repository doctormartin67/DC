libpath = /home/doctormartin67/Projects/library/
objs = test.o lifetable.o ${libpath}helperfunctions.o
testTables : ${objs}
	cc -g -o testTables ${objs}
test.o : test.c
	cc -g -c test.c
lifetable.o : lifetable.c
	cc -g -c -I${libpath} lifetable.c
helperfunctions.o : ${libpath}helperfunctions.c
	cc -g -c ${libpath}helperfunctions.c
