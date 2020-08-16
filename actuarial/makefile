libpath = /home/doctormartin67/Projects/library/
tablepath = /home/doctormartin67/Projects/work/tables/
objs = test.o ${tablepath}lifetable.o ${libpath}helperfunctions.o actuarialfunctions.o -lm
debug = #add -g if you want to debug
actfunc : ${objs}
	cc ${debug} -o actfunc ${objs}
test.o : test.c
	cc ${debug} -c test.c
lifetable.o : ${tablepath}lifetable.c
	cc ${debug} -c -I${libpath} ${tablepath}lifetable.c
helperfunctions.o : ${libpath}helperfunctions.c
	cc ${debug} -c ${libpath}helperfunctions.c
actuarialfunctions.o : actuarialfunctions.c
	cc ${debug} -c -I${tablepath} actuarialfunctions.c
