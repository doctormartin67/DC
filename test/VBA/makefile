# ---variables---
INCLUDE = ../../include
SRC = ../../src
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -pedantic -I $(INCLUDE)
OBJS = test.o common.o lex.o type.o ast.o print_ast.o parse.o resolve.o

# ---vpath---
vpath %.c $(SRC)
vpath %.h $(INCLUDE)

# ---commands---
RM = rm

# ---targets---
all: test run_msg 
run_msg:
	@printf "#\n# Running VBA interpreter test \n#\n"

test: $(OBJS)
test.o: lex.h common.h
common.o: common.h
lex.o: lex.h common.h
type.o: type.h common.h
ast.o: ast.h lex.h common.h
print_ast.o: ast.h
parse.o: parse.h ast.h lex.h common.h
resolve.o: resolve.h

.PHONY: clean
clean:
	$(RM) *.o
	$(RM) test
