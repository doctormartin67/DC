LIBPATH = /home/doctormartin67/Projects/library
TABLEPATH = /home/doctormartin67/Projects/work/tables
BINPATH = /home/doctormartin67/bin
libs = -lm #libraries to include: -lm is Math.
objs = ${LIBPATH}/helperfunctions.o
debug = #add -g if you want to debug
programs = createXLzip
SRCS = createXLzip.o

${programs} : %: %.o ${objs}
	gcc ${debug} -o $@ $< ${objs}
	mv $@ ${BINPATH}

${SRCS} : %.o: %.c
	gcc ${debug} -c $<

${LIBPATH}/helperfunctions.o : ${LIBPATH}/helperfunctions.c
	gcc ${debug} -c -I${LIBPATH} $< -o $@
${LIBPATH}/helperfunctions.o : ${LIBPATH}/libraryheader.h
