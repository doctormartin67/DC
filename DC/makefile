#***Variables***

CC = gcc
LIBPATH = /home/doctormartin67/Projects/library
TABLEPATH = /home/doctormartin67/Projects/work/tables
ACTPATH = /home/doctormartin67/Projects/work/actuarial
BINPATH = /home/doctormartin67/bin
GTK = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
LDFLAGS = -lm -lxlsxwriter #libraries to include: -lm is Math, -lxlsxwriter is an excel reader lib.
miscOBJS = ${TABLEPATH}/lifetables.o ${ACTPATH}/actuarialfunctions.o DCProgram.o client.o
libOBJS = ${LIBPATH}/helperfunctions.o ${LIBPATH}/XLfunctions.o ${LIBPATH}/hashtable.o
gtkOBJS = userinterface.o

# flags: 
# -g if you want to debug, 
# -pg for gprof
# -Wall -Wshadow - Wextra are warning flags
# -O0 -O1 -O2 -O3 are optimization flags
# -ffast-math see page 686 in C in a nutshell
# the OPTFLAGS should be removed for debugging. I also want to check whether with or without
# -ffast-math gives a big difference
OPTFLAGS = -O3 -ffast-math
CFLAGS = -g -pg -Wall -Wshadow -Wextra ${OPTFLAGS}
programs = main
SRCS = main.o

#***Start makefile***

${programs}: %: %.o ${miscOBJS} ${libOBJS} ${gtkOBJS}
	${CC} -o $@ $< ${miscOBJS} ${libOBJS} ${gtkOBJS} ${LDFLAGS} ${GTK}
	mv $@ ${BINPATH}
	cp ${LIBPATH}/runDC ${BINPATH}

${SRCS}: %.o: %.c ${ACTPATH}/actuarialfunctions.h ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h DCProgram.h
	${CC} -c -I${LIBPATH} -I${ACTPATH} -I${TABLEPATH} -I. $< ${CFLAGS} 

${TABLEPATH}/lifetables.o: %.o: %.c
	${CC} -c -I${LIBPATH} -I${TABLEPATH} $< -o $@ ${CFLAGS} 

${TABLEPATH}/lifetables.o: ${LIBPATH}/libraryheader.h ${TABLEPATH}/lifetables.h

${ACTPATH}/actuarialfunctions.o: %.o: %.c
	${CC} -c -I. -I${TABLEPATH} -I${LIBPATH} $< -o $@ ${CFLAGS} 

${ACTPATH}/actuarialfunctions.o: ${ACTPATH}/actuarialfunctions.h ${TABLEPATH}/lifetables.h DCProgram.h ${LIBPATH}/hashtable.h

${libOBJS}: %.o: %.c ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h
	${CC} -c -I${LIBPATH} $< -o $@ ${CFLAGS} 

DCProgram.o: %.o: %.c
	${CC} -c -I${LIBPATH} $< -o $@ ${CFLAGS}

DCProgram.o: ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h DCProgram.h

client.o: %.o: %.c
	${CC} -c -I${LIBPATH} -I${TABLEPATH} $< -o $@ ${CFLAGS} 

client.o: ${LIBPATH}/libraryheader.h ${LIBPATH}/hashtable.h DCProgram.h ${TABLEPATH}/lifetables.h

userinterface.o: %.o: %.c
	${CC} -c $< -o $@ ${GTK} ${CFLAGS} 

clean:
	rm ${BINPATH}/${programs} ${miscOBJS} ${libOBJS} ${gtkOBJS}
