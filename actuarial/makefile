LIBPATH = /home/doctormartin67/Projects/library
TABLEPATH = /home/doctormartin67/Projects/work/tables
BINPATH = /usr/local/bin
libs = -lm #libraries to include: -lm is Math.
objs = ${TABLEPATH}/lifetable.o ${LIBPATH}/helperfunctions.o actuarialfunctions.o ${libs}
debug = #add -g if you want to debug

axn : axntest.o ${objs}
	gcc ${debug} -o $@ axntest.o ${objs}
	mv axn ${BINPATH}
nEx : nExtest.o ${objs}
	gcc ${debug} -o $@ nExtest.o ${objs}
	mv nEx ${BINPATH}
npx : npxtest.o ${objs}
	gcc ${debug} -o $@ npxtest.o ${objs}
	mv npx ${BINPATH}

axntest.o : axntest.c
	gcc ${debug} -c axntest.c
axntest.o : actuarialfunctions.h

nExtest.o : nExtest.c
	gcc ${debug} -c nExtest.c
nExtest.o : actuarialfunctions.h

npxtest.o : npxtest.c
	gcc ${debug} -c npxtest.c
npxtest.o : actuarialfunctions.h

${TABLEPATH}/lifetable.o : ${TABLEPATH}/lifetable.c
	gcc ${debug} -c -I${LIBPATH} -I${TABLEPATH} ${TABLEPATH}/lifetable.c -o ${TABLEPATH}/lifetable.o
${TABLEPATH}/lifetable.o : ${LIBPATH}/libraryheader.h ${TABLEPATH}/lifetables.h

${LIBPATH}/helperfunctions.o : ${LIBPATH}/helperfunctions.c
	gcc ${debug} -c -I${LIBPATH} ${LIBPATH}/helperfunctions.c -o ${LIBPATH}/helperfunctions.o
${LIBPATH}/helperfunctions.o : ${LIBPATH}/libraryheader.h

actuarialfunctions.o : actuarialfunctions.c
	gcc ${debug} -c -I${TABLEPATH} actuarialfunctions.c
actuarialfunctions.o : actuarialfunctions.h ${TABLEPATH}/lifetables.h
