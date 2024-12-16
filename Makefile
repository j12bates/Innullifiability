CC		:= gcc
CCFLAGS	:= -Wall -Wextra -std=c17 -pthread

OPFLAGS	:= -O3 -flto -DNO_VALIDATE
DBFLAGS	:= -g -DDEBUG

OBJ		:= obj
TARGET	:= bin

SRC		:= util
SRC_X	:= extra
LIB		:= lib

OBJ_IFACE	:= $(OBJ)/iface.o
OBJ_SETREC	:= $(OBJ)/setRec.o
OBJ_EXPAND	:= $(OBJ)/expand.o
OBJ_NULTEST	:= $(OBJ)/nulTest.o

SRC_GEN		:= $(SRC)/generation.c
SRC_WEED	:= $(SRC)/weed.c
SRC_EVAL	:= $(SRC)/evaluate.c
SRC_CREATE	:= $(SRC)/create.c

DEP_UTIL	:= $(OBJ_IFACE) $(OBJ_SETREC)
DEP_GEN		:= $(OBJ_EXPAND)
DEP_WEED	:= $(OBJ_NULTEST)
DEP_EVAL	:=
DEP_CREATE	:=

GEN			:= $(TARGET)/gen
WEED		:= $(TARGET)/weed
EVAL		:= $(TARGET)/eval
CREATE		:= $(TARGET)/create

SRC_COUNT	:= $(SRC_X)/count.c
COUNT		:= $(TARGET)/count

UTILS		:= $(GEN) $(WEED) $(EVAL) $(CREATE)
EXTRA		:= $(COUNT)

.PHONY: all out debug clean utils dirs

all: out

out: CCFLAGS += $(OPFLAGS)
out: utils extra

debug: CCFLAGS += $(DBFLAGS)
debug: utils extra

utils: dirs $(UTILS)

extra: $(EXTRA)

dirs:
	mkdir -p $(OBJ) $(TARGET)

clean:
	rm -r $(OBJ) $(TARGET)

$(OBJ)/%.o: $(LIB)/%.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(GEN): $(DEP_GEN) $(SRC_GEN)
$(WEED): $(DEP_WEED) $(SRC_WEED)
$(EVAL): $(DEP_EVAL) $(SRC_EVAL)
$(CREATE): $(DEP_CREATE) $(SRC_CREATE)

$(COUNT): $(SRC_COUNT)

$(UTILS): $(DEP_UTIL)
	$(CC) $(CCFLAGS) $^ -o $@

$(EXTRA):
	$(CC) $(CCFLAGS) $^ -o $@
