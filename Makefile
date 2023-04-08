SRC 	:= main.c setRec.c setTree.c eqSets.c nulTest.c

CC		:= gcc
CCFLAGS	:= -Wall

OPFLAGS := -O3 -flto
DBFLAGS := -g

.PHONY: all debug clean

all: innullifiables

innullifiables: $(SRC)
	$(CC) $(CCFLAGS) $(OPFLAGS) $(SRC) -o $@

debug: $(SRC)
	$(CC) $(CCFLAGS) $(DBFLAGS) $(SRC) -o innullifiables

clean:
	rm innullifiables
