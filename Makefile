SRC 	:= main.c setTree.c eqSets.c nulTest.c

CC		:= gcc
CCFLAGS	:=

DBFLAGS := -g

.PHONY: all debug clean

all: innullifiables

innullifiables:
	$(CC) $(CCFLAGS) $(SRC) -o $@

debug: CCFLAGS += $(DBFLAGS)
debug: innullifiables

clean:
	rm innullifiables
