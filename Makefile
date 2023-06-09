OBJS 	:= setRec.o mutate.o supers.o nulTest.o

CC		:= gcc
CCFLAGS	:= -Wall -Wextra

OPFLAGS := -O2 -flto
DBFLAGS := -g -DDEBUG

.PHONY: all out debug clean

all: out

%.o: %/*.c
	$(CC) $(CCFLAGS) -c $< -o $@

innullifiables: main.c $(OBJS)
	$(CC) $(CCFLAGS) $^ -o $@

out: CCFLAGS += $(OPFLAGS)
out: innullifiables

debug: CCFLAGS += $(DBFLAGS)
debug: innullifiables

clean:
	rm innullifiables $(OBJS)
