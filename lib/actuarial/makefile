# ---variables---
INCLUDE = ../../include
SRC = ../../src
LIB = ../../lib
EXE = test

GPROF = #-pg

# ---Rule Structure---
CC = gcc
CFLAGS = $(GPROF) -g -Wall -Wextra -Werror -pedantic
INCLUDES = -I $(INCLUDE)
LIBS = -lgeneral -lm
LDLIBS = $(LIBS)
LDFLAGS = $(GPROF) -L$(LIB)
COMPILE.c = $(CC) $(CFLAGS) $(INCLUDES) $(CPPFLAGS) $(TARGET_ARCH) -c

# ---Object files---
OBJS = actfuncs.o 
.DELETE_ON_ERROR: $(OBJS)

# ---vpath---
vpath %.c $(SRC)
vpath %.h $(INCLUDE)

# ---commands---
RM = rm -rf
CP = cp

# ---targets---
all: $(EXE)

fast: CFLAGS += -Ofast
fast: all

$(EXE): $(EXE).o libactuarial.a

libactuarial.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $?
	$(CP) $@ ..

# include dependencies created below
include $(subst .o,.d,$(OBJS))

# trick to create dependencies, see make book page 33
%.d: %.c
	$(CC) -M $(INCLUDES) $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

.PHONY: clean
clean:
	$(RM) *.[oda]
	$(RM) $(EXE)
