SRC 	:= main.c setTree.c eqSets.c nulTest.c

CC		:= gcc
CCFLAGS	:= -O3 -flto

DBFLAGS := -g

.PHONY: all debug clean

all: innullifiables

innullifiables: $(SRC)
	$(CC) $(CCFLAGS) $(SRC) -o $@

debug: CCFLAGS += $(DBFLAGS)
debug: innullifiables

clean:
	rm innullifiables
