# ---variables---
INCLUDE = ../../include
SRC = ../../src
LIB = ../../lib
EXE = test
PACKAGE = libxml2-dev

# ---Rule Structure---
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -pedantic
INCLUDES = -I $(INCLUDE) `pkg-config --cflags libxml-2.0`
LIBS = -lgeneral
LDLIBS = $(LIBS) `xml2-config --libs`
LDFLAGS = -L$(LIB)
COMPILE.c = $(CC) $(CFLAGS) $(INCLUDES) $(CPPFLAGS) $(TARGET_ARCH) -c

# ---Object files---
OBJS = excel.o
.DELETE_ON_ERROR: $(OBJS)

# ---vpath---
vpath %.c $(SRC)
vpath %.h $(INCLUDE)

# ---commands---
RM = rm -rf
CP = cp

# ---targets---
all: $(PACKAGE) $(EXE)

$(PACKAGE):
	@echo "#####"
	@echo "Cheking if package '$(PACKAGE)' is installed"
	@echo "#####"
	@dpkg -s $(PACKAGE); \
	if [ $$? ]; then \
	    echo "#####"; \
	    echo "package '$(PACKAGE)' found"; \
	    echo "#####"; \
	else \
	    echo "package '$(PACKAGE)' not installed"; \
	    read -p "install? (y/n)" confirm; \
	    if [ $$confirm = y ]; then \
		sudo apt install $(PACKAGE); \
	    fi \
	fi
	
$(EXE): $(EXE).o libexcel.a

libexcel.a: $(OBJS)
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
