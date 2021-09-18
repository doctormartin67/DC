# commands:
CC = gcc
RM = rm
CP = cp
MV = mv

# paths:
PROJECTPATH = /home/doctormartin67/Projects
LIBPATH = $(PROJECTPATH)/library
TABLEPATH =
ACTPATH =
INTPATH = $(PROJECTPATH)/work/DC/interpreter
INCLUDE = -I. -I$(LIBPATH) -I$(INTPATH)
BINPATH = /home/doctormartin67/bin

# objects:
XML = `xml2-config --cflags` `xml2-config --libs`
GTK = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` -rdynamic
MISCOBJS = $(INTPATH)/treeerrors.o $(INTPATH)/interpreter.o
LIBOBJS = $(LIBPATH)/helperfunctions.o $(LIBPATH)/errorexit.o calculator.o
GTKOBJS =
EXEC = jcalc

# flags: 
# -g if you want to debug, 
# -pg for gprof
# -Wall -Wshadow - Wextra are warning flags
# -O0 -O1 -O2 -O3 are optimization flags
# -ffast-math see page 686 in C in a nutshell
# the OPTFLAGS should be removed for debugging. I also want to check whether with or without
# -ffast-math gives a big difference
OPTFLAGS =
DEBUGFLAGS = -g
GPROFFLAGS =
CFLAGS = -Wall -Wshadow -Wextra $(GPROFFLAGS)
LDFLAGS = -lm#libraries to include: -lm is Math, -lxlsxwriter is an excel reader lib.
#***Start makefile***

# phony targets:
.PHONY: debug clean

# targets:
$(EXEC) debug: $(EXEC).o $(MISCOBJS) $(LIBOBJS) $(GTKOBJS)
	$(CC) -o $(EXEC) $^ $(LDFLAGS) $(GPROFFLAGS) $(GTK) $(XML)
	$(CP) $(EXEC) $(BINPATH)

$(EXEC): CFLAGS += $(OPTFLAGS)

debug: CFLAGS += $(DEBUGFLAGS)

clean:
	$(RM) $(BINPATH)/$(EXEC) $(EXEC) $(EXEC).o $(MISCOBJS) $(LIBOBJS) $(GTKOBJS)

# dependencies:
%.o: %.c
	$(CC) $(INCLUDE) -c $< -o $@ $(GTK) $(XML) $(CFLAGS) 

# clever way to add all dependencies, see page 723 of C in a nutshell.
dependencies: $(EXEC).c $(MISCOBJS:.o=.c) $(LIBOBJS:.o=.c) $(GTKOBJS:.o=.c)
	$(CC) -M $(INCLUDE) $(GTK) $(XML) $^ > $@

include dependencies
