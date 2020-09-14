LIBPATH = /home/doctormartin67/Projects/library
TABLEPATH = /home/doctormartin67/Projects/work/tables
BINPATH = /home/doctormartin67/bin
libs = -lm #libraries to include: -lm is Math.
objs = ${TABLEPATH}/lifetable.o ${LIBPATH}/helperfunctions.o actuarialfunctions.o ${libs}
debug = #add -g if you want to debug
programs = axn nEx nPx Ax1n IAx1n Iaxn
SRCS = axntest.o nExtest.o nPxtest.o Ax1ntest.o IAx1ntest.o Iaxntest.o

${programs} : %: %test.o ${objs}
	gcc ${debug} -o $@ $< ${objs}
	mv $@ ${BINPATH}

${SRCS} : %.o: %.c actuarialfunctions.h
	gcc ${debug} -c $<

${TABLEPATH}/lifetable.o : ${TABLEPATH}/lifetable.c
	gcc ${debug} -c -I${LIBPATH} -I${TABLEPATH} $< -o $@
${TABLEPATH}/lifetable.o : ${LIBPATH}/libraryheader.h ${TABLEPATH}/lifetables.h

${LIBPATH}/helperfunctions.o : ${LIBPATH}/helperfunctions.c
	gcc ${debug} -c -I${LIBPATH} $< -o $@
${LIBPATH}/helperfunctions.o : ${LIBPATH}/libraryheader.h

actuarialfunctions.o : actuarialfunctions.c
	gcc ${debug} -c -I${TABLEPATH} $<
actuarialfunctions.o : actuarialfunctions.h ${TABLEPATH}/lifetables.h
