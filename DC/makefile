# commands:
CC = gcc
RM = rm
CP = cp
MV = mv

# paths:
LIBPATH = /home/doctormartin67/Projects/library
TABLEPATH = /home/doctormartin67/Projects/work/tables
ACTPATH = /home/doctormartin67/Projects/work/actuarial
INCLUDE = -I. -I$(LIBPATH) -I$(TABLEPATH) -I$(ACTPATH)
BINPATH = /home/doctormartin67/bin

# objects:
GTK = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
MISCOBJS = $(TABLEPATH)/lifetables.o $(ACTPATH)/actuarialfunctions.o DCProgram.o client.o
LIBOBJS = $(LIBPATH)/helperfunctions.o $(LIBPATH)/XLfunctions.o $(LIBPATH)/hashtable.o
GTKOBJS = userinterface.o
EXEC = main
SCRIPT = runDC

# flags: 
# -g if you want to debug, 
# -pg for gprof
# -Wall -Wshadow - Wextra are warning flags
# -O0 -O1 -O2 -O3 are optimization flags
# -ffast-math see page 686 in C in a nutshell
# the OPTFLAGS should be removed for debugging. I also want to check whether with or without
# -ffast-math gives a big difference
OPTFLAGS = -O3 -ffast-math
DEBUGFLAGS = -g
GPROFFLAGS = -pg
CFLAGS = -Wall -Wshadow -Wextra $(GPROFFLAGS)
LDFLAGS = -lm -lxlsxwriter#libraries to include: -lm is Math, -lxlsxwriter is an excel reader lib.

#***Start makefile***

# phony targets:
.PHONY: debug clean

# targets:
$(EXEC) debug: $(EXEC).o $(MISCOBJS) $(LIBOBJS) $(GTKOBJS)
	$(CC) -o $(EXEC) $^ $(LDFLAGS) $(GPROFFLAGS) $(GTK)
	$(MV) $(EXEC) $(BINPATH)
	$(CP) $(LIBPATH)/$(SCRIPT) $(BINPATH)

$(EXEC): CFLAGS += $(OPTFLAGS)

debug: CFLAGS += $(DEBUGFLAGS)

clean:
	$(RM) $(BINPATH)/$(EXEC) $(EXEC).o $(MISCOBJS) $(LIBOBJS) $(GTKOBJS)

# dependencies:
$(EXEC).o: %.o: %.c $(ACTPATH)/actuarialfunctions.h DCProgram.h
	$(CC) -c $(INCLUDE) $< $(CFLAGS) 

%.o: %.c
	$(CC) $(INCLUDE) -c $< -o $@ $(CFLAGS) 

$(TABLEPATH)/lifetables.o: %.o: %.h $(LIBPATH)/libraryheader.h

$(ACTPATH)/actuarialfunctions.o: %.o: %.h $(LIBPATH)/hashtable.h

$(LIBPATH)/helperfunctions.o: $(LIBPATH)/libraryheader.h 

$(LIBPATH)/XLfunctions.o: $(LIBPATH)/libraryheader.h 

$(LIBPATH)/hashtable.o: %.o: %.h $(LIBPATH)/libraryheader.h 

DCProgram.o: %.o: %.h $(LIBPATH)/libraryheader.h

client.o: DCProgram.h $(TABLEPATH)/lifetables.h

userinterface.o: %.o: %.c
	$(CC) -c $< -o $@ $(GTK) $(CFLAGS) 
