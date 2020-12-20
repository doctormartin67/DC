#***Variables***

LIBPATH = /home/doctormartin67/Projects/library
TABLEPATH = /home/doctormartin67/Projects/work/tables
ACTPATH = /home/doctormartin67/Projects/work/actuarial
BINPATH = /home/doctormartin67/bin
libs = -lm -lxlsxwriter #libraries to include: -lm is Math.
miscOBJS = ${TABLEPATH}/lifetable.o ${ACTPATH}/actuarialfunctions.o DCProgram.o client.o
libOBJS = ${LIBPATH}/helperfunctions.o ${LIBPATH}/XLfunctions.o ${LIBPATH}/hashtable.o

debug = #add -g if you want to debug
programs = main
SRCS = main.o

#***Start makefile***

${programs} : %: %.o ${miscOBJS} ${libOBJS}
	gcc ${debug} -o $@ $< ${miscOBJS} ${libOBJS} ${libs}
	mv $@ ${BINPATH}
	cp ${LIBPATH}/runDC ${BINPATH}

${SRCS} : %.o: %.c ${ACTPATH}/actuarialfunctions.h ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h DCProgram.h
	gcc ${debug} -c -I${LIBPATH} -I${ACTPATH} -I${TABLEPATH} -I. $<

${TABLEPATH}/lifetable.o : ${TABLEPATH}/lifetable.c
	gcc ${debug} -c -I${LIBPATH} -I${TABLEPATH} $< -o $@
${TABLEPATH}/lifetable.o : ${LIBPATH}/libraryheader.h ${TABLEPATH}/lifetables.h

${ACTPATH}/actuarialfunctions.o : ${ACTPATH}/actuarialfunctions.c
	gcc ${debug} -c -I. -I${TABLEPATH} -I${LIBPATH} $< -o $@
${ACTPATH}/actuarialfunctions.o : ${ACTPATH}/actuarialfunctions.h ${TABLEPATH}/lifetables.h DCProgram.h

${libOBJS} : %.o: %.c ${LIBPATH}/libraryheader.h
	gcc ${debug} -c -I${LIBPATH} $< -o $@

DCProgram.o : DCProgram.c
	gcc ${debug} -c -I${LIBPATH} $< -o $@
DCProgram.o : ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h DCProgram.h

client.o : client.c
	gcc ${debug} -c -I${LIBPATH} $< -o $@
client.o : ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h DCProgram.h
