# ---variables---
INCLUDE = ../../include
SRC = ../../src
LIB = ../../lib
EXE = test

# ---Rule Structure---
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -pedantic
INCLUDES = -I $(INCLUDE)
LIBS = -lgeneral
LDLIBS = $(LIBS)
LDFLAGS = -L$(LIB)
COMPILE.c = $(CC) $(CFLAGS) $(INCLUDES) $(CPPFLAGS) $(TARGET_ARCH) -c

# ---Object files---
OBJS = lex.o type.o ast.o print_ast.o parse.o resolve.o
.DELETE_ON_ERROR: $(OBJS)

# ---vpath---
vpath %.c $(SRC)
vpath %.h $(INCLUDE)

# ---commands---
RM = rm
CP = cp

# ---targets---
all: $(EXE)

$(EXE): $(EXE).o libvba.a

libvba.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $?
	$(CP) $@ ..

# include dependencies created below
include $(subst .o,.d,$(OBJS))

# trick to create dependencies, see make book page 33
%.d: %.c
	$(CC) -M $(INCLUDES) $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) -f $@.$$$$

.PHONY: clean
clean:
	$(RM) *.[oda]
	$(RM) $(EXE)
