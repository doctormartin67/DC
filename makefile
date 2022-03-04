# ---variables---
INCLUDE = include
SRC = src
LIB = lib
EXE = main

# ---dependencies---
GTK = libgtk-3-dev
ZLIB = zlib1g-dev

# ---Rule Structure---
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -pedantic
# THIS BELOW INCLUDRES AND LDLIBS ON XML CAN BE REMOVED
INCLUDES = -I $(INCLUDE) `pkg-config --cflags gtk+-3.0` \
	`pkg-config --cflags libxml-2.0`
LIBS = -lgeneral -lexcel -lxlsxwriter
LDLIBS = $(LIBS) `pkg-config --libs gtk+-3.0` `xml2-config --libs`
LDFLAGS = -L$(LIB)
COMPILE.c = $(CC) $(CFLAGS) $(INCLUDES) $(CPPFLAGS) $(TARGET_ARCH) -c

# ---Object files---
OBJS = actuarialfunctions.o assumptions.o dates.o DCProgram.o hashtable.o \
	lifetables.o printresults.o userinterface.o userrunhandlers.o \
	usersignalhandlers.o validation.o
.DELETE_ON_ERROR: $(OBJS)

# ---vpath---
vpath %.c $(SRC)
vpath %.h $(INCLUDE)

# ---commands---
RM = rm -rf
CP = cp

# ---targets---
all: DEPENDENCIES $(EXE)

DEPENDENCIES:
	$(call install_dependency, $(GTK))
	$(call install_dependency, $(ZLIB))
	$(call install_xlsxwriter, lxlsxwriter)
	
$(EXE): $(EXE).o $(OBJS)

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

# ---functions---
define install_dependency
	@echo "####################"
	@echo "Cheking if package '$1' is installed"
	@echo "####################"
	@if dpkg -s $1; then \
	    echo "####################"; \
	    echo "package '$1' found"; \
	    echo "####################"; \
	else \
	    echo "package '$1' not installed"; \
	    read -p "install? (y/n)" confirm; \
	    if [ $$confirm = y ]; then \
		sudo apt install $1; \
	    fi \
	fi
endef

define install_xlsxwriter
	@echo "####################"
	@echo "Cheking if library '$1' is installed"
	@echo "####################"
	@if ld -lxlsxwriter; then \
	    echo "####################"; \
	    echo "package '$1' found"; \
	    echo "####################"; \
	else \
	    echo "package '$1' not installed"; \
	    read -p "install? (y/n)" confirm; \
	    if [ $$confirm = y ]; then \
	        cd /tmp; git clone https://github.com/jmcnamara/libxlsxwriter.git; \
	        cd libxlsxwriter; make; sudo make install; \
	    fi \
	fi
endef	
