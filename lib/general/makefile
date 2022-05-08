# ---variables---
INCLUDE = ../../include
SRC = ../../src
EXE = test

# ---Rule Structure---
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -pedantic
INCLUDES = -I $(INCLUDE)
COMPILE.c = $(CC) $(CFLAGS) $(INCLUDES) $(CPPFLAGS) $(TARGET_ARCH) -c
LDFLAGS =

# ---Object files---
OBJS = common.o errorexit.o helperfunctions.o
.DELETE_ON_ERROR: $(OBJS)

# ---vpath---
vpath %.c $(SRC)
vpath %.h $(INCLUDE)

# ---commands---
RM = rm -rf
CP = cp

# ---targets---
all: $(EXE)

fast: CFLAGS = -g -Wextra -Werror -pedantic -Ofast
fast: all

$(EXE): $(EXE).o libgeneral.a

libgeneral.a: $(OBJS)
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
