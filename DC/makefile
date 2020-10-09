LIBPATH = /home/doctormartin67/Projects/library
TABLEPATH = /home/doctormartin67/Projects/work/tables
ACTPATH = /home/doctormartin67/Projects/work/actuarial
BINPATH = /home/doctormartin67/bin
libs = -lm #libraries to include: -lm is Math.
miscOBJS = ${TABLEPATH}/lifetable.o ${ACTPATH}/actuarialfunctions.o DCProgram.o
libOBJS = ${LIBPATH}/helperfunctions.o ${LIBPATH}/XLfunctions.o ${LIBPATH}/hashtable.o

debug = #add -g if you want to debug
programs = main
SRCS = main.o

${programs} : %: %.o ${miscOBJS} ${libOBJS} ${libs}
	gcc ${debug} -o $@ $< ${miscOBJS} ${libOBJS} ${libs}
	mv $@ ${BINPATH}
	cp ${LIBPATH}/runDC ${BINPATH}

${SRCS} : %.o: %.c ${ACTPATH}/actuarialfunctions.h ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h DCProgram.h
	gcc ${debug} -c $< -I${LIBPATH} -I${ACTPATH}

${TABLEPATH}/lifetable.o : ${TABLEPATH}/lifetable.c
	gcc ${debug} -c -I${LIBPATH} -I${TABLEPATH} $< -o $@
${TABLEPATH}/lifetable.o : ${LIBPATH}/libraryheader.h ${TABLEPATH}/lifetables.h

${ACTPATH}/actuarialfunctions.o : ${ACTPATH}/actuarialfunctions.c
	gcc ${debug} -c -I${TABLEPATH} $< -o $@
${ACTPATH}/actuarialfunctions.o : ${ACTPATH}/actuarialfunctions.h ${TABLEPATH}/lifetables.h

${libOBJS} : %.o: %.c ${LIBPATH}/libraryheader.h
	gcc ${debug} -c -I${LIBPATH} $< -o $@

DCProgram.o : DCProgram.c
	gcc ${debug} -c -I${LIBPATH} $< -o $@
DCProgram.o : DCProgram.h ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h
