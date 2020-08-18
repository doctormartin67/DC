libpath = /home/doctormartin67/Projects/library/
tablepath = /home/doctormartin67/Projects/work/tables/
objs = ${tablepath}lifetable.o ${libpath}helperfunctions.o actuarialfunctions.o -lm
debug = #add -g if you want to debug
axn : axntest.o ${objs}
	cc ${debug} -o axn axntest.o ${objs}
nEx : nExtest.o ${objs}
	cc ${debug} -o nEx nExtest.o ${objs}
npx : npxtest.o ${objs}
	cc ${debug} -o npx npxtest.o ${objs}
axntest.o : axntest.c
	cc ${debug} -c axntest.c
nExtest.o : nExtest.c
	cc ${debug} -c nExtest.c
npxtest.o : npxtest.c
	cc ${debug} -c npxtest.c
lifetable.o : ${tablepath}lifetable.c
	cc ${debug} -c -I${libpath} ${tablepath}lifetable.c
helperfunctions.o : ${libpath}helperfunctions.c
	cc ${debug} -c ${libpath}helperfunctions.c
actuarialfunctions.o : actuarialfunctions.c
	cc ${debug} -c -I${tablepath} actuarialfunctions.c
